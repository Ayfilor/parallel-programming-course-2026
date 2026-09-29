struct Test_Context {
    const char *package_name;
    const char *file_name;
    const char *test_name;
    bool failed;
    bool report_failure;
};

struct Test_Abort {};

internal Test_Context *test__current_context;

typedef void Test_Proc();

template <typename Tag>
struct Test_Definition {
    static constexpr Test_Proc *proc = nullptr;
};

struct Test_Case {
    const char *name;
    Test_Proc *proc;
};

struct Test_File {
    const char *name;
    const Test_Case *tests;
    u64 test_count;
};

struct Test_Package {
    const char *name;
    const Test_File *files;
    u64 file_count;
};

#define array_count(array) (sizeof(array) / sizeof((array)[0]))

#define test__name_2(scope, name) scope##__##name
#define test__name_1(scope, name) test__name_2(scope, name)
#define test__name(name) test__name_1(TEST_SCOPE, name)

#define test__tag_2(scope, name) scope##__test_tag__##name
#define test__tag_1(scope, name) test__tag_2(scope, name)
#define test__tag(name) test__tag_1(TEST_SCOPE, name)

#define test(name)                                                         \
    struct test__tag(name) {};                                             \
    internal void test__name(name)();                                      \
    template <>                                                            \
    struct Test_Definition<test__tag(name)> {                              \
        static constexpr Test_Proc *proc = &test__name(name);              \
    };                                                                     \
    internal void test__name(name)()

#define test_case(name)                                                    \
    {#name, Test_Definition<struct test__tag(name)>::proc}
#define test_file(name, tests) {#name, tests, array_count(tests)}
#define test_package(name, files) {#name, files, array_count(files)}

internal void
test__print_name(std::ostream &out, const Test_Context &context)
{
    out << context.package_name << '/' << context.file_name << '/'
        << context.test_name;
}

internal bool
test__matches(std::string_view wanted, const char *actual)
{
    return wanted.empty() || wanted == actual;
}

internal bool
test__same_name(const char *left, const char *right)
{
    return left && right && std::string_view(left) == right;
}

internal bool
test__validate(const Test_Package *packages, u64 package_count)
{
    u64 error_count = 0;

    for (u64 package_index = 0; package_index < package_count; ++package_index) {
        const Test_Package &package = packages[package_index];
        if (!package.name) {
            std::cerr << "test configuration error: package "
                      << package_index << " has no name\n";
            ++error_count;
            continue;
        }
        if (package.file_count != 0 && !package.files) {
            std::cerr << "test configuration error: package " << package.name
                      << " has no file table\n";
            ++error_count;
            continue;
        }

        for (u64 previous = 0; previous < package_index; ++previous) {
            if (test__same_name(package.name, packages[previous].name)) {
                std::cerr << "test configuration error: duplicate package: "
                          << package.name << '\n';
                ++error_count;
                break;
            }
        }

        for (u64 file_index = 0; file_index < package.file_count; ++file_index) {
            const Test_File &file = package.files[file_index];
            if (!file.name) {
                std::cerr << "test configuration error: file " << file_index
                          << " in package " << package.name
                          << " has no name\n";
                ++error_count;
                continue;
            }
            if (file.test_count != 0 && !file.tests) {
                std::cerr << "test configuration error: " << package.name
                          << '/' << file.name << " has no test table\n";
                ++error_count;
                continue;
            }

            for (u64 previous = 0; previous < file_index; ++previous) {
                if (test__same_name(file.name, package.files[previous].name)) {
                    std::cerr << "test configuration error: duplicate file: "
                              << package.name << '/' << file.name << '\n';
                    ++error_count;
                    break;
                }
            }

            for (u64 test_index = 0; test_index < file.test_count; ++test_index) {
                const Test_Case &current = file.tests[test_index];
                if (!current.name) {
                    std::cerr << "test configuration error: invalid test entry "
                              << test_index << " in " << package.name << '/'
                              << file.name << '\n';
                    ++error_count;
                    continue;
                }
                if (!current.proc) {
                    std::cerr << "test configuration error: unknown test: "
                              << package.name << '/' << file.name << '/'
                              << current.name << '\n'
                              << "    define test(" << current.name
                              << ") or remove test_case(" << current.name
                              << ")\n";
                    ++error_count;
                    continue;
                }

                for (u64 previous = 0; previous < test_index; ++previous) {
                    const Test_Case &other = file.tests[previous];
                    if (test__same_name(current.name, other.name) ||
                        current.proc == other.proc) {
                        std::cerr << "test configuration error: duplicate test: "
                                  << package.name << '/' << file.name << '/'
                                  << current.name << '\n';
                        ++error_count;
                        break;
                    }
                }
            }
        }
    }

    if (error_count != 0)
        std::cerr << "test configuration has " << error_count
                  << (error_count == 1 ? " error\n" : " errors\n");
    return error_count == 0;
}

internal int
test_run(int argc, char **argv, const Test_Package *packages, u64 package_count)
{
    constexpr u64 failure_report_limit = 10;
    std::string_view package_filter;
    std::string_view file_filter;

    if (argc > 2) {
        std::cerr << "usage: " << argv[0] << " [package[/file]]\n";
        return 2;
    }

    if (!test__validate(packages, package_count))
        return 2;

    if (argc == 2) {
        std::string_view filter = argv[1];
        auto slash = filter.find('/');
        package_filter = filter.substr(0, slash);
        if (slash != std::string_view::npos)
            file_filter = filter.substr(slash + 1);

        if (package_filter.empty() ||
            (slash != std::string_view::npos && file_filter.empty()) ||
            file_filter.find('/') != std::string_view::npos) {
            std::cerr << "filter must be package or package/file\n";
            return 2;
        }
    }

    u64 selected_count = 0;
    u64 failed_count = 0;
    bool matched_filter = false;

    for (u64 package_index = 0; package_index < package_count; ++package_index) {
        const Test_Package &package = packages[package_index];
        if (!test__matches(package_filter, package.name))
            continue;

        for (u64 file_index = 0; file_index < package.file_count; ++file_index) {
            const Test_File &file = package.files[file_index];
            if (!test__matches(file_filter, file.name))
                continue;

            matched_filter = true;
            for (u64 test_index = 0; test_index < file.test_count; ++test_index) {
                const Test_Case &current = file.tests[test_index];
                ++selected_count;

                Test_Context context = {
                    package.name,
                    file.name,
                    current.name,
                    false,
                    failed_count < failure_report_limit,
                };
                test__current_context = &context;
                try {
                    current.proc();
                } catch (const Test_Abort &) {
                }
                test__current_context = nullptr;

                if (context.failed)
                    ++failed_count;
            }
        }
    }

    if (!matched_filter) {
        std::cerr << "no package or file matched";
        if (!package_filter.empty()) {
            std::cerr << ": " << package_filter;
            if (!file_filter.empty())
                std::cerr << '/' << file_filter;
        }
        std::cerr << '\n';
        return 1;
    }

    if (failed_count > failure_report_limit)
        std::cerr << failed_count - failure_report_limit
                  << " more failed tests not shown\n";

    std::cout << selected_count - failed_count << " passed, "
              << failed_count << " failed\n";
    return failed_count == 0 ? 0 : 1;
}

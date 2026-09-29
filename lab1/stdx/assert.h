internal inline void
assert__print_diagnostics(std::ostream &)
{
}

template <typename Name, typename Value, typename... Rest>
internal inline void
assert__print_diagnostics(std::ostream &out, const Name &name,
                          const Value &value, const Rest &...rest)
{
    out << "    " << name << ": " << value << '\n';
    assert__print_diagnostics(out, rest...);
}

template <typename Message, typename... Diagnostics>
[[noreturn]] internal inline void
assert__failed(const char *file, int line, const char *function,
               const char *expression, const Message &message,
               const Diagnostics &...diagnostics)
{
    static_assert(sizeof...(diagnostics) % 2 == 0,
                  "assert arguments must be name/value pairs");

#if defined(BUILD_TESTS)
    if (test__current_context) {
        test__current_context->failed = true;
        if (test__current_context->report_failure) {
            std::cerr << "FAIL ";
            test__print_name(std::cerr, *test__current_context);
            std::cerr << '\n'
                      << "    " << file << ':' << line << ": " << function
                      << ": assertion failed: " << expression << '\n'
                      << "        " << message << '\n';
            assert__print_diagnostics(std::cerr, diagnostics...);
        }
        throw Test_Abort{};
    }
#endif

    std::cerr << file << ':' << line << ": " << function
              << ": assertion failed: " << expression << '\n'
              << "    " << message << '\n';
    assert__print_diagnostics(std::cerr, diagnostics...);
    std::exit(EXIT_FAILURE);
}

#undef assert
#define assert(condition, message, ...)                                     \
    do {                                                                    \
        if (!(condition)) {                                                 \
            assert__failed(__FILE__, __LINE__, __func__, #condition,        \
                           message __VA_OPT__(,) __VA_ARGS__);              \
        }                                                                   \
    } while (0)

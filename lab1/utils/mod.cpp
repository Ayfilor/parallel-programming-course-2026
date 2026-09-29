#include "zipf_generator.cpp"
#include "percentile.cpp"


#if defined(BUILD_TESTS)
Test_File utils_test_files[] = {
    test_file(zipf_generator, zipf_generator_tests),
};
#endif

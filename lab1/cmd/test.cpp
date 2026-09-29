#define BUILD_TESTS
#include "../imports.h"

int
main(int argc, char **argv)
{
    Test_Package packages[] = {
        test_package(utils, utils_test_files),
    };

    return test_run(argc, argv, packages, array_count(packages));
}

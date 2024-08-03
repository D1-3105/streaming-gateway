#include "gtest/gtest.h"
#include "logging.h"

int main(int argc, char **argv)
{
    logging::init_logging();
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

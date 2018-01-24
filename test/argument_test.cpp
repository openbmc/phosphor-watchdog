#include <string>

#include "argument.hpp"
#include "argument_test.hpp"

static const std::string expected_path1 = "/arg1-test-path";
static const std::string expected_target1 = "t1.target";

// Allow for a single unrecognized option then the Usage printout
static const std::string invalid_arg_regex =
        "^[^\n]*unrecognized option[^\n]*\nUsage: ";

static const std::string clean_usage_regex = "^Usage: ";

namespace phosphor
{
namespace watchdog
{

void ArgumentTest::SetUp()
{
    arg0 = "argument_test";
}

/** @brief ArgumentParser should return no values if given no options */
TEST_F(ArgumentTest, NoOptions)
{
    char * const args[] = {
        &arg0[0], nullptr
    };
    ArgumentParser ap(sizeof(args)/sizeof(char *) - 1, args);
    EXPECT_EQ(ArgumentParser::emptyString, ap["path"]);
    EXPECT_EQ(ArgumentParser::emptyString, ap["continue"]);
    EXPECT_EQ(ArgumentParser::emptyString, ap["arbitrary_unknown"]);
}

/** @brief ArgumentParser should return true for an existing no-arg option
 *         Make sure we don't parse arguments if an option takes none
 *         Also make sure we don't populate options not used.
 */
TEST_F(ArgumentTest, LongOptionNoArg)
{
    std::string arg_continue = "--continue";
    std::string arg_extra = "not-a-bool";
    char * const args[] = {
        &arg0[0], &arg_continue[0], &arg_extra[0], nullptr
    };
    ArgumentParser ap(sizeof(args)/sizeof(char *) - 1, args);
    EXPECT_EQ(ArgumentParser::emptyString, ap["path"]);
    EXPECT_EQ(ArgumentParser::trueString, ap["continue"]);
}

/** @brief ArgumentParser should return a string for long options that
 *         take an arg
 */
TEST_F(ArgumentTest, LongOptionRequiredArg)
{
    std::string arg_path = "--path";
    std::string arg_path_val = expected_path1;
    std::string arg_extra = "/unused-path";
    char * const args[] = {
        &arg0[0], &arg_path[0], &arg_path_val[0], &arg_extra[0], nullptr
    };
    ArgumentParser ap(sizeof(args)/sizeof(char *) - 1, args);
    EXPECT_EQ(expected_path1, ap["path"]);
}

/** @brief ArgumentParser should return a string for long options that
 *         accept an argument when passed an argument inline
 */
TEST_F(ArgumentTest, LongOptionInlineArg)
{
    std::string arg_path = "--path=" + expected_path1;
    std::string arg_extra = "/unused-path";
    char * const args[] = {
        &arg0[0], &arg_path[0], &arg_extra[0], nullptr
    };
    ArgumentParser ap(sizeof(args)/sizeof(char *) - 1, args);
    EXPECT_EQ(expected_path1, ap["path"]);
}

/** @brief ArgumentParser should return a string for short options that
 *         accept an argument.
 */
TEST_F(ArgumentTest, ShortOptionRequiredArg)
{
    std::string arg_path = "-p";
    std::string arg_path_val = expected_path1;
    std::string arg_extra = "/unused-path";
    char * const args[] = {
        &arg0[0], &arg_path[0], &arg_path_val[0], &arg_extra[0], nullptr
    };
    ArgumentParser ap(sizeof(args)/sizeof(char *) - 1, args);
    EXPECT_EQ(expected_path1, ap["path"]);
}

/** @brief ArgumentParser should be able to handle parsing multiple options
 *         Make sure that when passed multiple of the same option it uses
 *         the argument to the option passed last
 *         Make sure this works for no-arg and required-arg type options
 *         Make sure this works between short and long options
 */
TEST_F(ArgumentTest, MultiOptionOverride)
{
    std::string arg_continue_short = "-c";
    std::string arg_path = "--path=" + expected_path1;
    std::string arg_continue_long = "--continue";
    std::string arg_target = "--target=/unused-path";
    std::string arg_target_short = "-t";
    std::string arg_target_val = expected_target1;
    char * const args[] = {
        &arg0[0], &arg_continue_short[0], &arg_path[0], &arg_continue_long[0],
        &arg_target[0], &arg_target_short[0], &arg_target_val[0], nullptr
    };
    ArgumentParser ap(sizeof(args)/sizeof(char *) - 1, args);
    EXPECT_EQ(expected_path1, ap["path"]);
    EXPECT_EQ(ArgumentParser::trueString, ap["continue"]);
    EXPECT_EQ(expected_target1, ap["target"]);
}

/** @brief ArgumentParser should print usage information when given a help
 *         argument anywhere in the arguments array
 */
TEST_F(ArgumentTest, ShortOptionHelp)
{
    std::string arg_extra = "extra";
    std::string arg_help = "-h";
    char * const args[] = {
        &arg0[0], &arg_extra[0], &arg_help[0], nullptr
    };
    EXPECT_EXIT(ArgumentParser(sizeof(args)/sizeof(char *) - 1, args),
            ::testing::ExitedWithCode(255), clean_usage_regex);
}

/** @brief ArgumentParser should print usage information when given a help
 *         argument anywhere in the arguments array
 */
TEST_F(ArgumentTest, LongOptionHelp)
{
    std::string arg_help = "--help";
    std::string arg_extra = "extra";
    char * const args[] = {
        &arg0[0], &arg_help[0], &arg_extra[0], nullptr
    };
    EXPECT_EXIT(ArgumentParser(sizeof(args)/sizeof(char *) - 1, args),
            ::testing::ExitedWithCode(255), clean_usage_regex);
}

/** @brief ArgumentParser should print an invalid argument error and
 *         usage information when given an invalid argument anywhere
 *         in the arguments array
 */
TEST_F(ArgumentTest, InvalidOptionHelp)
{
    std::string arg_continue = "--continue";
    std::string arg_bad = "--bad_arg";
    std::string arg_target = "--target=/unused-path";
    char * const args[] = {
        &arg0[0], &arg_continue[0], &arg_bad[0], &arg_target[0], nullptr
    };
    EXPECT_EXIT(ArgumentParser(sizeof(args)/sizeof(char *) - 1, args),
            ::testing::ExitedWithCode(255), invalid_arg_regex);
}

}  // namespace watchdog
}  // namespace phosphor

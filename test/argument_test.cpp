#include <string>

#include "argument.hpp"
#include "argument_test.hpp"

namespace phosphor
{
namespace watchdog
{

void ArgumentTest::SetUp()
{
    arg0 = "argument_test";
}

TEST_F(ArgumentTest, EmptyArgs)
{
    char * const args[] = {
        &arg0[0], nullptr
    };
    ArgumentParser ap(sizeof(args)/sizeof(char *) - 1, args);
    EXPECT_EQ(ArgumentParser::emptyString, ap["path"]);
    EXPECT_EQ(ArgumentParser::emptyString, ap["continue"]);
    EXPECT_EQ(ArgumentParser::emptyString, ap["arbitrary_unknown"]);
}

TEST_F(ArgumentTest, BoolArg)
{
    std::string arg_continue = "--continue";
    std::string arg_extra = "extra";
    char * const args[] = {
        &arg0[0], &arg_continue[0], &arg_extra[0], nullptr
    };
    ArgumentParser ap(sizeof(args)/sizeof(char *) - 1, args);
    EXPECT_EQ(ArgumentParser::emptyString, ap["path"]);
    EXPECT_EQ(ArgumentParser::trueString, ap["continue"]);
}

TEST_F(ArgumentTest, StringArgLong)
{
    const std::string expected_path = "/arg-test-path";
    std::string arg_path = "--path";
    std::string arg_path_val = expected_path;
    std::string arg_extra = "extra";
    char * const args[] = {
        &arg0[0], &arg_path[0], &arg_path_val[0], &arg_extra[0], nullptr
    };
    ArgumentParser ap(sizeof(args)/sizeof(char *) - 1, args);
    EXPECT_EQ(expected_path, ap["path"]);
    EXPECT_EQ(ArgumentParser::emptyString, ap["continue"]);
}

TEST_F(ArgumentTest, StringArgLongCombined)
{
    const std::string expected_path = "/arg-test-path";
    std::string arg_path = "--path=" + expected_path;
    std::string arg_extra = "extra";
    char * const args[] = {
        &arg0[0], &arg_path[0], &arg_extra[0], nullptr
    };
    ArgumentParser ap(sizeof(args)/sizeof(char *) - 1, args);
    EXPECT_EQ(expected_path, ap["path"]);
    EXPECT_EQ(ArgumentParser::emptyString, ap["continue"]);
}

TEST_F(ArgumentTest, StringArgShort)
{
    const std::string expected_path = "/arg-test-path";
    std::string arg_path = "-p";
    std::string arg_path_val = expected_path;
    std::string arg_extra = "extra";
    char * const args[] = {
        &arg0[0], &arg_path[0], &arg_path_val[0], &arg_extra[0], nullptr
    };
    ArgumentParser ap(sizeof(args)/sizeof(char *) - 1, args);
    EXPECT_EQ(expected_path, ap["path"]);
    EXPECT_EQ(ArgumentParser::emptyString, ap["continue"]);
}

TEST_F(ArgumentTest, MultiArg)
{
    const std::string expected_path = "/arg-test-path";
    const std::string expected_target = "reboot-host.target";
    std::string arg_continue_short = "-c";
    std::string arg_path = "--path=" + expected_path;
    std::string arg_continue_long = "--continue";
    std::string arg_target = "--target=bad.target";
    std::string arg_target_short = "-t";
    std::string arg_target_val = expected_target;
    char * const args[] = {
        &arg0[0], &arg_continue_short[0], &arg_path[0], &arg_continue_long[0],
        &arg_target[0], &arg_target_short[0], &arg_target_val[0], nullptr
    };
    ArgumentParser ap(sizeof(args)/sizeof(char *) - 1, args);
    EXPECT_EQ(expected_path, ap["path"]);
    EXPECT_EQ(ArgumentParser::trueString, ap["continue"]);
    EXPECT_EQ(expected_target, ap["target"]);
}

TEST_F(ArgumentTest, HelpShort)
{
    std::string arg_extra = "extra";
    std::string arg_help = "-h";
    char * const args[] = {
        &arg0[0], &arg_extra[0], &arg_help[0], nullptr
    };
    EXPECT_EXIT(ArgumentParser(sizeof(args)/sizeof(char *) - 1, args),
            ::testing::ExitedWithCode(255), "^Usage: ");
}

TEST_F(ArgumentTest, HelpLong)
{
    std::string arg_extra = "extra";
    std::string arg_help = "--help";
    char * const args[] = {
        &arg0[0], &arg_extra[0], &arg_help[0], nullptr
    };
    EXPECT_EXIT(ArgumentParser(sizeof(args)/sizeof(char *) - 1, args),
            ::testing::ExitedWithCode(255), "^Usage: ");
}

TEST_F(ArgumentTest, HelpBadArg)
{
    std::string arg_extra = "extra";
    std::string arg_help = "--bad_arg";
    char * const args[] = {
        &arg0[0], &arg_extra[0], &arg_help[0], nullptr
    };
    EXPECT_EXIT(ArgumentParser(sizeof(args)/sizeof(char *) - 1, args),
            ::testing::ExitedWithCode(255), "Usage: ");
}

}  // namespace watchdog
}  // namespace phosphor

#ifndef ARGUMENT_TEST_HPP
#define ARGUMENT_TEST_HPP

#include <string>

#include <gtest/gtest.h>

namespace phosphor
{
namespace watchdog
{

class ArgumentTest : public testing::Test
{
    protected:
        void SetUp() override;

        std::string arg0;
};

}  // namespace watchdog
}  // namespace phosphor

#endif  // ARGUMENT_TEST_HPP

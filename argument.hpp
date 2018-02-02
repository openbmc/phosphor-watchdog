#pragma once

#include <getopt.h>
#include <map>
#include <string>
#include <vector>

namespace phosphor
{
namespace watchdog
{
/** @brief Class ArgumentParser - Encapsulates parsing command line options
 *                                and populating arguments
 */
class ArgumentParser
{
    public:
        ArgumentParser() = delete;
        ~ArgumentParser() = default;
        ArgumentParser(const ArgumentParser&) = delete;
        ArgumentParser& operator=(const ArgumentParser&) = delete;
        ArgumentParser(ArgumentParser&&) = default;
        ArgumentParser& operator=(ArgumentParser&&) = default;

        /** @brief Contructs ArgumentParser object
         *
         *  @param argc - the main function's argc passed as is
         *  @param argv - the main function's argv passed as is
         *  @return Object constructed
         */
        ArgumentParser(int argc, char * const argv[]);

        /** @brief Given an option, returns its argument(optarg)
         *
         *  @param opt - command line option string
         *
         *  @return argument which is a standard optarg
         */
        const std::vector<std::string>& operator[](const std::string& opt);

        /** @brief Displays usage
         *
         *  @param argv - the main function's argv passed as is
         */
        static void usage(char * const argv[]);

        /** @brief Set to 'true' when an option is passed */
        static const std::string trueString;

    private:
        /** @brief Option to argument mapping */
        std::map<const std::string, std::vector<std::string> > arguments;

        /** @brief Array of struct options as needed by getopt_long */
        static const option options[];

        /** @brief optstring as needed by getopt_long */
        static const char* optionStr;
};

} // namespace watchdog
} // namespace phosphor

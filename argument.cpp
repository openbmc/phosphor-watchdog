/**
 * Copyright © 2016 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <cassert>
#include "argument.hpp"

namespace phosphor
{
namespace watchdog
{

using namespace std::string_literals;

const std::vector<std::string> emptyArg;
const std::string ArgumentParser::trueString = "true"s;

const char* ArgumentParser::optionStr = "p:s:t:a:ch";
const option ArgumentParser::options[] =
{
    { "path",          required_argument,  nullptr,   'p' },
    { "service",       required_argument,  nullptr,   's' },
    { "target",        required_argument,  nullptr,   't' },
    { "action_target", required_argument,  nullptr,   'a' },
    { "continue",      no_argument,        nullptr,   'c' },
    { "help",          no_argument,        nullptr,   'h' },
    { 0, 0, 0, 0},
};

ArgumentParser::ArgumentParser(int argc, char * const argv[])
{
    int option;
    int opt_idx = 0;

    // We have to reset optind because getopt_long keeps global state
    // and uses optind to track what argv index it needs to process next.
    // Since this object may be instantiated more than once or test suites may
    // already process instructions, optind may not be initialized to point to
    // the beginning of our argv.
    optind = 0;
    while (-1 != (option = getopt_long(argc, argv, optionStr, options, &opt_idx)))
    {
        if (option == '?' || option == 'h')
        {
            usage(argv);
            exit(-1);
        }

        auto i = &options[opt_idx];
        while (i->val && i->val != option)
        {
            ++i;
        }

        if (i->val)
        {
            arguments[i->name].push_back(optarg ? optarg : trueString);
        }

        opt_idx = 0;
    }
}

const std::vector<std::string>& ArgumentParser::operator[](const std::string& opt)
{
    auto i = arguments.find(opt);
    if (i == arguments.end())
    {
        return emptyArg;
    }
    else
    {
        return i->second;
    }
}

void ArgumentParser::usage(char * const argv[])
{
    std::cerr << "Usage: " << argv[0] << " options\n";
    std::cerr << "Options:\n";
    std::cerr << " --help                                    Print this menu\n";
    std::cerr << " --path=<Dbus Object path>                 Dbus Object path. "
                     "Ex: /xyz/openbmc_project/state/watchdog/host0\n";
    std::cerr << " --service=<Dbus Service name>             Dbus Service "
                     "name. Ex: xyz.openbmc_project.State.Watchdog.Host\n";
    std::cerr << " [--target=<systemd unit>]                 Systemd unit to "
                     "be called on timeout for all actions but NONE. "
                      "Deprecated, use --action_target instead.\n";
    std::cerr << " [--action_target=<action>=<systemd unit>] Map of action to "
                     "systemd unit to be called on timeout if that action is "
                     "set for ExpireAction when the timer expires.\n";
    std::cerr << " [--continue]                              Continue daemon "
                     "after watchdog timeout.\n";
}
} // namespace watchdog
} // namespace phosphor

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
#include <iterator>
#include <algorithm>
#include <cassert>
#include "argument.hpp"

namespace phosphor
{
namespace watchdog
{

using namespace std::string_literals;

const std::string ArgumentParser::trueString = "true"s;
const std::string ArgumentParser::emptyString = ""s;

const char* ArgumentParser::optionStr = "p:s:t:?h";
const option ArgumentParser::options[] =
{
    { "path",     required_argument,  nullptr,   'o' },
    { "service",  required_argument,  nullptr,   's' },
    { "target",   required_argument,  nullptr,   't' },
    { "help",     no_argument,        nullptr,   'h' },
    { 0, 0, 0, 0},
};

ArgumentParser::ArgumentParser(int argc, char** argv)
{
    int option = 0;
    while (-1 != (option = getopt_long(argc, argv, optionStr, options, nullptr)))
    {
        if ((option == '?') || (option == 'h'))
        {
            usage(argv);
            exit(-1);
        }

        auto i = &options[0];
        while ((i->val != option) && (i->val != 0))
        {
            ++i;
        }

        if (i->val)
        {
            arguments[i->name] = (i->has_arg ? optarg : trueString);
        }
    }
}

const std::string& ArgumentParser::operator[](const std::string& opt)
{
    auto i = arguments.find(opt);
    if (i == arguments.end())
    {
        return emptyString;
    }
    else
    {
        return i->second;
    }
}

void ArgumentParser::usage(char** argv)
{
    std::cerr << "Usage: " << argv[0] << " [options]\n";
    std::cerr << "Options:\n";
    std::cerr << " --help                        Print this menu\n";
    std::cerr << " --path=<Dbus Object path>     Dbus Object path."
                                                 " Ex: /xyz/openbmc_project/"
                                                 "state/watchdog/host0\n";
    std::cerr << " --service=<Dbus Service name> Dbus Service name."
                                                 " Ex: xyz.openbmc_project."
                                                 "State.Watchdog.Host\n";
    std::cerr << " --target=<systemd unit>       Systemd unit to be called on"
                                                 " timeout\n";
}
} // namespace watchdog
} // namespace phosphor

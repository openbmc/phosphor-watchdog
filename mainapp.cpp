/**
 * Copyright © 2017 IBM Corporation
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
#include "argument.hpp"

static void exitWithError(const char* err, char** argv)
{
    phosphor::watchdog::ArgumentParser::usage(argv);
    std::cerr << "ERROR: " << err << "\n";
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    // Read arguments.
    auto options = phosphor::watchdog::ArgumentParser(argc, argv);

    // Parse out path argument.
    auto path = (options)["path"];
    if (path == phosphor::watchdog::ArgumentParser::emptyString)
    {
        exitWithError("Path not specified.", argv);
    }

    // Parse out service name argument
    auto service = (options)["service"];
    if (service == phosphor::watchdog::ArgumentParser::emptyString)
    {
        exitWithError("Service not specified.", argv);
    }

    // Parse out target argument. It is fine if the caller does not
    // pass this if they are not interested in calling into any target
    // on meeting a condition.
    auto target = (options)["target"];

    return 0;
}

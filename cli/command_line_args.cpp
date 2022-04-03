/*
 * fuzzuf
 * Copyright (C) 2021 Ricerca Security
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */
#include "fuzzuf/cli/command_line_args.hpp"
#include "fuzzuf/logger/logger.hpp"

namespace fuzzuf::cli {

std::vector<std::string> CommandLineArgs::Args() {
    std::vector<std::string> res;
    for (int i = 0; i < argc; i += 1) {
        if (argv[i] == nullptr) break;
        // DEBUG("[*] CommandLineArgs::Args(): argv[%d] = %s", i, argv[i]);
        res.push_back(std::string(argv[i]));
    }
    return res;
}

} // namespace fuzzuf::cli

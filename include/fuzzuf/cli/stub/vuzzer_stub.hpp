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
#pragma once

#include <vector>
#include <string>
#include <memory>
#include "fuzzuf/utils/common.hpp"
#include "fuzzuf/fuzzer/fuzzer.hpp"
#include "fuzzuf/algorithms/vuzzer/vuzzer_state.hpp"

namespace fuzzuf::cli::stub {

template<class State>
class VUzzerStub : public Fuzzer {
public:

    std::unique_ptr<State> state;

    VUzzerStub(
        std::unique_ptr<State>&& state
    )
      : state(std::move(state))
    {}

    void ReceiveStopSignal(void) {}
    bool ShouldEnd(void) { return false; }
};

} // namespace fuzzuf::cli::stub

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
/**
 * @file VUzzerUpdateHierarFlowRoutines.hpp
 * @brief HieraFlow nodes for update methods
 * @author Ricerca Security <fuzzuf-dev@ricsec.co.jp>
 */
#pragma once

#include <memory>
#include "fuzzuf/algorithms/vuzzer/vuzzer_state.hpp"
#include "fuzzuf/algorithms/vuzzer/vuzzer_testcase.hpp"
#include "fuzzuf/algorithms/vuzzer/vuzzer_util.hpp"

#include "fuzzuf/feedback/file_feedback.hpp"
#include "fuzzuf/feedback/exit_status_feedback.hpp"

#include "fuzzuf/hierarflow/hierarflow_routine.hpp"
#include "fuzzuf/hierarflow/hierarflow_node.hpp"
#include "fuzzuf/hierarflow/hierarflow_intermediates.hpp"

namespace fuzzuf::algorithm::vuzzer::routine::update {

using VUzzerUpdInputType = double(const std::shared_ptr<VUzzerTestcase>&, FileFeedback&);
using VUzzerUpdCalleeRef = NullableRef<HierarFlowCallee<VUzzerUpdInputType>>;
using VUzzerUpdOutputType = void(void);

struct UpdateFitness
    : public HierarFlowRoutine<
        VUzzerUpdInputType,
        void(const std::shared_ptr<VUzzerTestcase>&, std::map<u64, u32>&)
    > {
public:
    UpdateFitness(VUzzerState &state);

    VUzzerUpdCalleeRef operator()(const std::shared_ptr<VUzzerTestcase>&, FileFeedback&);

private:
    VUzzerState &state;
};

struct UpdateTaint
    : public HierarFlowRoutine<
        VUzzerUpdInputType,
        VUzzerUpdOutputType
    > {
public:
    UpdateTaint(VUzzerState &state);

    VUzzerUpdCalleeRef operator()(const std::shared_ptr<VUzzerTestcase>&, FileFeedback&);

private:
    VUzzerState &state;
};

struct TrimQueue
    : public HierarFlowRoutine<
        void(const std::shared_ptr<VUzzerTestcase>&, std::map<u64, u32>&),
        VUzzerUpdOutputType
    > {
public:
    TrimQueue(VUzzerState &state);

    NullableRef<HierarFlowCallee<void(const std::shared_ptr<VUzzerTestcase>&, std::map<u64, u32>&)>> operator()(const std::shared_ptr<VUzzerTestcase>&, std::map<u64, u32>&);

private:
    VUzzerState &state;
};

struct UpdateQueue
    : public HierarFlowRoutine<
        void(void),
        void(void)
    > {
public:
    UpdateQueue(VUzzerState &state);

    NullableRef<HierarFlowCallee<void(void)>> operator()(void);

private:
    VUzzerState &state;
};

} // namespace fuzzuf::algorithm::vuzzer::routine::update

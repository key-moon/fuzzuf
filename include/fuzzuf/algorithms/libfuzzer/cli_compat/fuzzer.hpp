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
 * @file fuzzer.hpp
 * @author Ricerca Security <fuzzuf-dev@ricsec.co.jp>
 */
#ifndef FUZZUF_INCLUDE_ALGORITHM_LIBFUZZER_FUZZER_HPP
#define FUZZUF_INCLUDE_ALGORITHM_LIBFUZZER_FUZZER_HPP

#include "fuzzuf/algorithms/libfuzzer/cli_compat/variables.hpp"
#include "fuzzuf/algorithms/libfuzzer/dictionary.hpp"
#include "fuzzuf/algorithms/libfuzzer/mutation_history.hpp"
#include "fuzzuf/algorithms/libfuzzer/state/common_types.hpp"
#include "fuzzuf/algorithms/libfuzzer/state/corpus.hpp"
#include "fuzzuf/algorithms/libfuzzer/state/state.hpp"
#include "fuzzuf/cli/fuzzer_args.hpp"
#include "fuzzuf/fuzzer/fuzzer.hpp"
#include "fuzzuf/utils/node_tracer.hpp"
#include "fuzzuf/utils/range_traits.hpp"
#include "fuzzuf/utils/type_traits/replace_return_type.hpp"
#include <chrono>
#include <cstddef>
#include <functional>
#include <random>
#include <vector>

namespace fuzzuf::cli {

struct GlobalFuzzerOptions;

} // namespace fuzzuf::cli

namespace fuzzuf::algorithm::libfuzzer {

class LibFuzzer : public ::Fuzzer {
  using Func = bool(Variables &, utils::DumpTracer &,
                    utils::ElapsedTimeTracer &);
  using Wrapped = utils::type_traits::replace_return_type_t<void, Func>;

public:
  LibFuzzer(fuzzuf::cli::FuzzerArgs &, const fuzzuf::cli::GlobalFuzzerOptions &,
            std::function<void(std::string &&)> &&);
  virtual ~LibFuzzer() {}
  virtual void OneLoop();
  virtual void ReceiveStopSignal(void) {}
  bool ShouldEnd() override { return end_; }
  const FuzzerCreateInfo &get_create_info() const { return create_info; }

private:
  FuzzerCreateInfo create_info;
  Variables vars;
  signed long long int total_cycles = 0u;
  std::size_t max_input_length = 0u;
  bool print_final_stats = false;
  bool end_ = false;
  std::function<void()> runone;
  std::function<void(std::string &&)> sink;
  utils::DumpTracer node_tracer;
  utils::ElapsedTimeTracer ett;
};
} // namespace fuzzuf::algorithm::libfuzzer

#endif

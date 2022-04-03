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
 * @file fuzzer.cpp
 * @author Ricerca Security <fuzzuf-dev@ricsec.co.jp>
 */
#include "fuzzuf/algorithms/libfuzzer/cli_compat/fuzzer.hpp"
#include "fuzzuf/algorithms/libfuzzer/cli_compat/options.hpp"
#include "fuzzuf/algorithms/libfuzzer/config.hpp"
#include "fuzzuf/algorithms/libfuzzer/create.hpp"
#include "fuzzuf/cli/fuzzer_args.hpp"
#include "fuzzuf/cli/global_fuzzer_options.hpp"
#include "fuzzuf/logger/logger.hpp"
#include <boost/program_options.hpp>
#include <cstdint>
#include <fstream>
#include <string>

namespace fuzzuf::algorithm::libfuzzer {

LibFuzzer::LibFuzzer(fuzzuf::cli::FuzzerArgs &fuzzer_args,
                     const fuzzuf::cli::GlobalFuzzerOptions &global,
                     std::function<void(std::string &&)> &&sink_)
    : node_tracer([this](std::string &&m) { sink("trace : " + m); }) {
  Options opts;
  opts.output_dir = global.out_dir;
  auto [desc, pd] = createOptions(opts);

  if (!postProcess(fuzzer_args.global_options_description.add(desc), pd, fuzzer_args.argc, fuzzer_args.argv, global,
                   std::move(sink_), opts)) {
    end_ = true;
    return;
  }
  create_info = opts.create_info;
  vars.state.config = opts.create_info.config;
  vars.rng = std::move(opts.rng);

  ExecInputSet initial_inputs = loadInitialInputs(opts, vars.rng);
  vars.max_input_size =
      opts.create_info.len_control ? 4u : opts.create_info.max_input_length;
  sink = std::move(opts.sink);
  total_cycles = opts.total_cycles;
  print_final_stats = opts.print_final_stats;

  vars.begin_date = std::chrono::system_clock::now();
  {
    auto root = createInitialize<Func, Order>(opts.targets[0], opts.create_info,
                                              initial_inputs, false, sink);

    namespace hf = fuzzuf::hierarflow;
    hf::WrapToMakeHeadNode(root)(vars, node_tracer, ett);
  }
  auto runone_ = createRunone<Func, Order>(opts.targets[0], opts.create_info,
                                           initial_inputs, sink);
  namespace hf = fuzzuf::hierarflow;
  auto runone_wrapped = hf::WrapToMakeHeadNode(runone_);
  runone = [this, runone_wrapped = std::move(runone_wrapped)]() mutable {
    runone_wrapped(vars, node_tracer, ett);
  };

  DEBUG("[*] LibFuzzer::LibFuzzer(): Done");
}
void LibFuzzer::OneLoop() {
  // DEBUG("[*] LibFuzzer::OneLoop(): end_: %s", end_ ? "true" : "false");
  if (!end_) {
    runone();
    if (total_cycles >= 0 && vars.count >= std::size_t(total_cycles)) {
      end_ = true;
      if (print_final_stats) {
        std::string message;
        utils::toStringADL(message, vars.state, 0, "  ");
        sink(std::move(message));
        ett.dump(sink);
      }
    }
  }
}

} // namespace fuzzuf::algorithm::libfuzzer

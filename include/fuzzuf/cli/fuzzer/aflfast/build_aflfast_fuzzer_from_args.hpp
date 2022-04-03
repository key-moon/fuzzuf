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

#include "fuzzuf/cli/put_args.hpp"
#include "fuzzuf/exceptions.hpp"
#include "fuzzuf/utils/optparser.hpp"
#include "fuzzuf/utils/workspace.hpp"
#include "fuzzuf/algorithms/aflfast/aflfast_option.hpp"
#include "fuzzuf/algorithms/aflfast/aflfast_setting.hpp"
#include "fuzzuf/algorithms/aflfast/aflfast_state.hpp"
#include "fuzzuf/executor/native_linux_executor.hpp"
#include "fuzzuf/executor/qemu_executor.hpp"
#ifdef __aarch64__
#include "fuzzuf/executor/coresight_executor.hpp"
#endif
#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace fuzzuf::cli::fuzzer::aflfast {

struct AFLFastFuzzerOptions {
    bool forksrv;                           // Optional
    bool frida_mode;                        // Optional

    // Default values
    AFLFastFuzzerOptions() : 
        forksrv(true),
        frida_mode(false)
        {};
};

// Fuzzer specific help
// TODO: Provide better help message
static void usage(po::options_description &desc) {
    std::cout << "Help:" << std::endl;
    std::cout << desc << std::endl;
    exit(1);
}

// Used only for CLI
template <class TFuzzer, class TAFLFuzzer, class TExecutor>
std::unique_ptr<TFuzzer> BuildAFLFastFuzzerFromArgs(
    fuzzuf::cli::FuzzerArgs &fuzzer_args, 
    GlobalFuzzerOptions &global_options
) {
    po::positional_options_description pargs_desc;
    pargs_desc.add("fuzzer", 1);
    pargs_desc.add("pargs", -1);

    AFLFastFuzzerOptions aflfast_options;

    po::options_description fuzzer_desc("AFLFast options");
    std::vector<std::string> pargs;
    fuzzer_desc.add_options()
        ("forksrv", 
            po::value<bool>(&aflfast_options.forksrv)->default_value(aflfast_options.forksrv), 
            "Enable/disable fork server mode. default is true.")
        // If you want to add fuzzer specific options, add options here
        ("pargs", 
            po::value<std::vector<std::string>>(&pargs), 
            "Specify PUT and args for PUT.")
        ("frida",
            po::value<bool>(&aflfast_options.frida_mode)->default_value(aflfast_options.frida_mode),
            "Enable/disable frida mode. Default to false.")
    ;

    po::variables_map vm;
    po::store(
        po::command_line_parser(fuzzer_args.argc, fuzzer_args.argv)
            .options(fuzzer_args.global_options_description.add(fuzzer_desc))
            .positional(pargs_desc)
            .run(), 
        vm
        );
    po::notify(vm);

    if (global_options.help) {
        fuzzuf::cli::fuzzer::aflfast::usage(fuzzer_args.global_options_description);
    }

    using fuzzuf::algorithm::aflfast::option::AFLFastTag;
    using fuzzuf::algorithm::afl::option::GetMemLimit;

    u32 mem_limit = global_options.exec_memlimit.value_or(GetMemLimit<AFLFastTag>());
    if (aflfast_options.frida_mode) {
        setenv("__AFL_DEFER_FORKSRV", "1", 1);
        fs::path frida_bin = fs::path(fuzzer_args.argv[0]).parent_path() / "afl-frida-trace.so";
        setenv("LD_PRELOAD", frida_bin.c_str(), 1);

        if (mem_limit > 0) {
            struct stat statbuf;
            if ((stat(frida_bin.c_str(), &statbuf)) == -1) {
                std::cerr << cLRD <<
                    "[-] File afl-frida-trace.so not found\n" <<
                    "    Build one first with cmake where -DENABLE_FRIDA_TRACE=1" <<
                    cRST << std::endl;
            }
            // Need to add the size of the library
            mem_limit += statbuf.st_size;
        }
    }

    PutArgs put(pargs);
    try {
        put.Check();
    } catch (const exceptions::cli_error &e) {
        std::cerr << "[!] " << e.what() << std::endl;
        std::cerr << "\tat " << e.file << ":" << e.line << std::endl;
        fuzzuf::cli::fuzzer::aflfast::usage(fuzzer_args.global_options_description);
    } 

    // Trace level log
    DEBUG("[*] PUT: put = [");
    for (auto v : put.Args()) {
        DEBUG("\t\"%s\",", v.c_str());
    }
    DEBUG("    ]");

    using fuzzuf::algorithm::aflfast::AFLFastSetting;
    using fuzzuf::algorithm::afl::option::GetExecTimeout;
    using fuzzuf::algorithm::aflfast::option::FAST;

    // Create AFLFastSetting

    auto setting = std::make_shared<const AFLFastSetting>(
                        put.Args(),
                        global_options.in_dir,
                        global_options.out_dir,
                        global_options.exec_timelimit_ms.value_or(GetExecTimeout<AFLFastTag>()),
                        mem_limit,
                        aflfast_options.forksrv,
                        /* dumb_mode */ false,  // FIXME: add dumb_mode
                        Util::CPUID_BIND_WHICHEVER,
                        FAST
                    );

    // NativeLinuxExecutor needs the directory specified by "out_dir" to be already set up
    // so we need to create the directory first, and then initialize Executor
    SetupDirs(setting->out_dir.string());

    using fuzzuf::algorithm::afl::option::GetDefaultOutfile;
    using fuzzuf::algorithm::afl::option::GetMapSize;
    using fuzzuf::cli::ExecutorKind;

    std::shared_ptr<TExecutor> executor;
    switch (global_options.executor) {
    case ExecutorKind::NATIVE: {
        auto nle = std::make_shared<fuzzuf::executor::NativeLinuxExecutor>(
                            setting->argv,
                            setting->exec_timelimit_ms,
                            setting->exec_memlimit,
                            setting->forksrv,
                            setting->out_dir / GetDefaultOutfile<AFLFastTag>(),
                            GetMapSize<AFLFastTag>(), // afl_shm_size
                            0 // bb_shm_size
                        );
        executor = std::make_shared<TExecutor>(std::move(nle));
        break;
    }

    case ExecutorKind::QEMU: {
        // NOTE: Assuming GetMapSize<AFLFastTag>() == QEMUExecutor::QEMU_SHM_SIZE
        auto qe = std::make_shared<fuzzuf::executor::QEMUExecutor>(
                            global_options.proxy_path.value(),
                            setting->argv,
                            setting->exec_timelimit_ms,
                            setting->exec_memlimit,
                            setting->forksrv,
                            setting->out_dir / GetDefaultOutfile<AFLFastTag>()
                        );
        executor = std::make_shared<TExecutor>(std::move(qe));
        break;
    }

#ifdef __aarch64__
    case ExecutorKind::CORESIGHT: {
        auto cse = std::make_shared<fuzzuf::executor::CoreSightExecutor>(
                            global_options.proxy_path.value(),
                            setting->argv,
                            setting->exec_timelimit_ms,
                            setting->exec_memlimit,
                            setting->forksrv,
                            setting->out_dir / GetDefaultOutfile<AFLFastTag>(),
                            GetMapSize<AFLFastTag>() // afl_shm_size
                        );
        executor = std::make_shared<TExecutor>(std::move(cse));
        break;
    }
#endif

    default:
        EXIT("Unsupported executor: '%s'", global_options.executor.c_str());
    }

    // Create AFLFastState
    using fuzzuf::algorithm::aflfast::AFLFastState;
    auto state = std::make_unique<AFLFastState>(setting, executor);

    return std::unique_ptr<TFuzzer>(
                dynamic_cast<TFuzzer *>(
                    new TAFLFuzzer(std::move(state))
                )
            );
}

} // namespace fuzzuf::cli::fuzzer::aflfast

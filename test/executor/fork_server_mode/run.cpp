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
#define BOOST_TEST_MODULE native_linux_executor.native_linux_context.fork.run
#define BOOST_TEST_DYN_LINK

#include <boost/scope_exit.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

#include "fuzzuf/algorithms/afl/afl_option.hpp"
#include "fuzzuf/exceptions.hpp"
#include "fuzzuf/executor/native_linux_executor.hpp"
#include "fuzzuf/feedback/inplace_memory_feedback.hpp"
#include "fuzzuf/feedback/put_exit_reason_type.hpp"
#include "fuzzuf/utils/common.hpp"
#include "fuzzuf/utils/filesystem.hpp"
#include "fuzzuf/utils/which.hpp"

// NativeLinuxExecutor::Run() の正常系テスト
BOOST_AUTO_TEST_CASE(NativeLinuxExecutorNativeRun) {
  // Setup root directory
  std::string root_dir_template("/tmp/fuzzuf_test.XXXXXX");
  auto *const raw_dirname = mkdtemp(root_dir_template.data());
  BOOST_CHECK(raw_dirname != nullptr);
  auto root_dir = fs::path(raw_dirname);
  BOOST_SCOPE_EXIT(&root_dir) { fs::remove_all(root_dir); }
  BOOST_SCOPE_EXIT_END

  // Create input/output dirctory
  auto input_dir = root_dir / "input";
  auto output_dir = root_dir / "output";
  BOOST_CHECK_EQUAL(fs::create_directory(input_dir), true);
  BOOST_CHECK_EQUAL(fs::create_directory(output_dir), true);

  // Setup output file path
  auto output_file_path = output_dir / "result";
  std::cout << "[*] output_file_path = " + output_file_path.native()
            << std::endl;

  // Create executor instance
  long val = sysconf(_SC_PAGESIZE);
  BOOST_CHECK(val != -1); // Make sure sysconf succeeds
  u32 PAGE_SIZE = (u32)val;

  auto path_to_write_seed = output_dir / "cur_input";
  fuzzuf::executor::NativeLinuxExecutor executor(
      {TEST_BINARY_DIR "/put_binaries/command_wrapper", "/bin/cat", "@@"}, 1000,
      10000, true, path_to_write_seed, PAGE_SIZE, PAGE_SIZE,
      true /* record_stdout_and_err */
  );
  BOOST_CHECK_EQUAL(executor.stdin_mode, false);

  // Invoke NativeLinuxExecutor::Run()
  std::string input("hello\n");
  executor.Run(reinterpret_cast<const u8 *>(input.c_str()), input.size());

  // Check normality
  // (1) 正常実行されたこと → feedbackのexit_reason が
  // PUTExitReasonType::FAULT_NONE であることを確認する
  BOOST_CHECK_EQUAL(executor.GetExitStatusFeedback().exit_reason,
                    PUTExitReasonType::FAULT_NONE);

  // (2) 標準入力によってファズが受け渡されたこと →
  // 標準入力と同じ内容がファイルに保存されたことを確認する
  auto stdout_buffer_feedback = executor.GetStdOut();
  stdout_buffer_feedback.ShowMemoryToFunc([](const u8 *ptr, u32 len) {
    std::vector<std::uint8_t> expected_stdout{'h', 'e', 'l', 'l', 'o', '\n'};

    BOOST_CHECK_EQUAL_COLLECTIONS(ptr, ptr + len, expected_stdout.begin(),
                                  expected_stdout.end());
  });
  InplaceMemoryFeedback::DiscardActive(std::move(stdout_buffer_feedback));

  auto stderr_buffer_feedback = executor.GetStdErr();
  stderr_buffer_feedback.ShowMemoryToFunc([](const u8 *ptr, u32 len) {
    std::vector<std::uint8_t> expected_stderr{};

    BOOST_CHECK_EQUAL_COLLECTIONS(ptr, ptr + len, expected_stderr.begin(),
                                  expected_stderr.end());
  });
  InplaceMemoryFeedback::DiscardActive(std::move(stderr_buffer_feedback));

  // (3) Check if executor correctly clears stdout_buffer before a new
  // execution. Otherwise the output from stdout during the previous execution
  // should remain in stdout_buffer.
  executor.Run(reinterpret_cast<const u8 *>(input.c_str()), input.size());

  stdout_buffer_feedback = executor.GetStdOut();
  stdout_buffer_feedback.ShowMemoryToFunc([](const u8 *ptr, u32 len) {
    std::vector<std::uint8_t> expected_stdout{'h', 'e', 'l', 'l', 'o', '\n'};

    BOOST_CHECK_EQUAL_COLLECTIONS(ptr, ptr + len, expected_stdout.begin(),
                                  expected_stdout.end());
  });
}

// Check if NativeLinuxExecutor can time out an execution of a PUT without going
// into a busy loop even if the PUT infinitely outputs strings.
BOOST_AUTO_TEST_CASE(NativeLinuxExecutorNativeRunTooMuchOutput,
                     *boost::unit_test::timeout(2)) {
  // Setup root directory
  std::string root_dir_template("/tmp/fuzzuf_test.XXXXXX");

  const auto raw_dirname = mkdtemp(root_dir_template.data());
  if (!raw_dirname)
    throw -1;
  BOOST_CHECK(raw_dirname != nullptr);

  auto root_dir = fs::path(raw_dirname);
  BOOST_SCOPE_EXIT(&root_dir) { fs::remove_all(root_dir); }
  BOOST_SCOPE_EXIT_END

  // Create input/output dirctory
  auto input_dir = root_dir / "input";
  auto output_dir = root_dir / "output";
  BOOST_CHECK_EQUAL(fs::create_directory(input_dir), true);
  BOOST_CHECK_EQUAL(fs::create_directory(output_dir), true);

  // Create executor instance
  long val = sysconf(_SC_PAGESIZE);
  BOOST_CHECK(val != -1); // Make sure sysconf succeeds
  u32 PAGE_SIZE = (u32)val;

  auto path_to_write_seed = output_dir / "cur_input";
  fuzzuf::executor::NativeLinuxExecutor executor(
      {TEST_BINARY_DIR "/put_binaries/command_wrapper",
       TEST_BINARY_DIR "/executor/too_much_output"},
      1000, 10000, true, path_to_write_seed, PAGE_SIZE, PAGE_SIZE,
      true /* record_stdout_and_err */
  );
  BOOST_CHECK_EQUAL(executor.stdin_mode, true);

  // Invoke NativeLinuxExecutor::Run()
  std::string input;
  executor.Run(reinterpret_cast<const u8 *>(input.c_str()), input.size());
  BOOST_CHECK_EQUAL(executor.GetExitStatusFeedback().exit_reason,
                    PUTExitReasonType::FAULT_TMOUT);
  BOOST_CHECK_EQUAL(executor.GetExitStatusFeedback().signal, SIGKILL);
}

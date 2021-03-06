/**
 *  Copyright (c) 2014-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed as defined on the LICENSE file found in the
 *  root directory of this source tree.
 */

#pragma once

#include <osquery/events/linux/probes/ebpf_tracepoint.h>
#include <osquery/events/linux/probes/syscall_event.h>
#include <osquery/events/linux/probes/syscalls_programs.h>

#include <osquery/utils/enum_class_hash.h>
#include <osquery/utils/expected/expected.h>
#include <osquery/utils/system/linux/ebpf/perf_output.h>

namespace osquery {
namespace events {

class LinuxProbesControl final {
 public:
  enum class Error {
    SystemUnknown = 1,
    SystemEbpf = 2,
    SystemNativeEvent = 3,
    SystemTracepoint = 4,
    SystemPerfEvent = 5,
    InvalidArgument = 6,
  };

  static Expected<LinuxProbesControl, LinuxProbesControl::Error> spawn();

  ebpf::PerfOutputsPoll<events::syscall::Event>& getReader();

  ExpectedSuccess<Error> traceKill();
  ExpectedSuccess<Error> traceSetuid();

 private:
  using PerfEventCpuMap = ebpf::Map<int, int, BPF_MAP_TYPE_PERF_EVENT_ARRAY>;

  explicit LinuxProbesControl(
      PerfEventCpuMap cpu_to_perf_output_map,
      ebpf::PerfOutputsPoll<events::syscall::Event> output_poll);

  ExpectedSuccess<Error> traceEnterAndExit(syscall::EventType type);

 private:
  std::unordered_map<syscall::EventType, EbpfTracepoint, EnumClassHash> probes_;
  PerfEventCpuMap cpu_to_perf_output_map_;
  ebpf::PerfOutputsPoll<events::syscall::Event> output_poll_;
};

} // namespace events
} // namespace osquery

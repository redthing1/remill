/*
 * Copyright (c) 2026-present Trail of Bits, Inc.
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

#pragma once

#include <gtest/gtest.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <remill/Arch/Name.h>
#include <remill/BC/Optimizer.h>
#include <remill/BC/TraceLifter.h>
#include <test_runner/TestRunner.h>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include <remill/Arch/MIPS/Runtime/State.h>

namespace mips::test {

template <remill::ArchName kArch>
struct ArchTraits;

template <>
struct ArchTraits<remill::ArchName::kArchMIPS32LittleEndian> {
  static uint64_t FetchPC(MIPSState *st) {
    return static_cast<uint64_t>(st->pc.dword);
  }

  static void SetPC(MIPSState *st, uint64_t pc) {
    st->pc.dword = static_cast<uint32_t>(pc);
  }
};

template <>
struct ArchTraits<remill::ArchName::kArchMIPS64LittleEndian> {
  static uint64_t FetchPC(MIPSState *st) {
    return st->pc.qword;
  }

  static void SetPC(MIPSState *st, uint64_t pc) {
    st->pc.qword = pc;
  }
};

template <remill::ArchName kArch>
inline void ExecuteOne(test_runner::LiftingTester &lifter,
                       std::string_view name, std::string bytes,
                       uint64_t addr, MIPSState *st,
                       test_runner::MemoryHandler *mem) {
  auto maybe = lifter.LiftInstructionFunction(name, bytes, addr);
  ASSERT_TRUE(maybe.has_value());

  auto *lifted_func = maybe->first;
  const auto &insn = maybe->second;

  auto optimized_mod = llvm::CloneModule(*lifted_func->getParent());
  remill::OptimizeBareModule(optimized_mod.get());

  auto just_func_mod =
      std::make_unique<llvm::Module>("", optimized_mod->getContext());
  auto *func = test_runner::CopyFunctionIntoNewModule(
      just_func_mod.get(), lifted_func, optimized_mod);

  test_runner::ExecuteLiftedFunction<MIPSState, test_runner::MemoryHandler>(
      func, insn.bytes.size(), st, mem,
      [](MIPSState *s) { return ArchTraits<kArch>::FetchPC(s); });
}

class MapTraceManager final : public remill::TraceManager {
 public:
  explicit MapTraceManager(std::unordered_map<uint64_t, uint8_t> memory_)
      : memory(std::move(memory_)) {}

  void SetLiftedTraceDefinition(uint64_t addr,
                                llvm::Function *lifted_func) override {
    traces[addr] = lifted_func;
  }

  llvm::Function *GetLiftedTraceDeclaration(uint64_t addr) override {
    auto it = traces.find(addr);
    return (it != traces.end()) ? it->second : nullptr;
  }

  llvm::Function *GetLiftedTraceDefinition(uint64_t addr) override {
    return GetLiftedTraceDeclaration(addr);
  }

  bool TryReadExecutableByte(uint64_t addr, uint8_t *byte) override {
    auto it = memory.find(addr);
    if (it == memory.end()) {
      return false;
    }
    *byte = it->second;
    return true;
  }

  std::unordered_map<uint64_t, uint8_t> memory;
  std::unordered_map<uint64_t, llvm::Function *> traces;
};

template <remill::ArchName kArch>
inline void ExecuteTrace(std::string_view trace_name,
                         std::unordered_map<uint64_t, uint8_t> bytes,
                         uint64_t addr, MIPSState *st,
                         test_runner::MemoryHandler *mem) {
  MapTraceManager manager(std::move(bytes));

  llvm::LLVMContext context;
  auto arch = remill::Arch::Build(&context, remill::OSName::kOSLinux, kArch);
  auto module = remill::LoadArchSemantics(arch.get());
  ASSERT_NE(module.get(), nullptr);

  remill::TraceLifter trace_lifter(arch.get(), &manager);
  ASSERT_TRUE(trace_lifter.Lift(addr));

  auto *trace_func = manager.GetLiftedTraceDefinition(addr);
  ASSERT_NE(trace_func, nullptr);
  trace_func->setName(std::string(trace_name));

  auto optimized_mod = llvm::CloneModule(*trace_func->getParent());
  remill::OptimizeBareModule(optimized_mod.get());

  auto just_func_mod =
      std::make_unique<llvm::Module>("", optimized_mod->getContext());
  auto *func = test_runner::CopyFunctionIntoNewModule(
      just_func_mod.get(), trace_func, optimized_mod);

  test_runner::ExecuteLiftedFunction<MIPSState, test_runner::MemoryHandler>(
      func, /*insn_length=*/4, st, mem,
      [](MIPSState *s) { return ArchTraits<kArch>::FetchPC(s); });
}

}  // namespace mips::test


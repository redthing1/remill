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

#include <gtest/gtest.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/Endian.h>
#include <remill/Arch/Name.h>
#include <remill/Arch/Runtime/HyperCall.h>
#include <remill/OS/OS.h>
#include <test_runner/TestRunner.h>

#include "TestHarness.h"
#include "TestUtil.h"

#include <cstdint>
#include <unordered_map>

namespace {
static constexpr remill::ArchName kArch =
    remill::ArchName::kArchMIPS64LittleEndian;
}  // namespace

TEST(MIPS64EL, ZeroWriteIsIgnored_Daddiu) {
  llvm::LLVMContext context;
  test_runner::LiftingTester lifter(context, remill::OSName::kOSLinux, kArch);

  const uint64_t addr = 0x1000;

  // daddiu $zero, $zero, 1 => opcode=0x19 rs=0 rt=0 imm=1
  const auto word = mips::EncodeI(/*opcode=*/0x19U, /*rs=*/0U, /*rt=*/0U,
                                  /*imm16=*/1);

  MIPSState st = {};
  st.pc.qword = addr;
  st.gpr.zero.qword = 0xDEADBEEF'CAFEBABEu;

  test_runner::MemoryHandler mem(llvm::endianness::little);
  mips::test::ExecuteOne<kArch>(lifter, "mips64_daddiu_zero_zero_1",
                               mips::Bytes32(word), addr, &st, &mem);

  EXPECT_EQ(st.gpr.zero.qword, 0u);
  EXPECT_EQ(st.pc.qword, addr + 4);
}

TEST(MIPS64EL, Daddiu_Daddu_Dsubu) {
  llvm::LLVMContext context;
  test_runner::LiftingTester lifter(context, remill::OSName::kOSLinux, kArch);

  const uint64_t addr = 0x2000;

  test_runner::MemoryHandler mem(llvm::endianness::little);
  MIPSState st = {};
  st.pc.qword = addr;

  // daddiu $t0, $zero, 5 => opcode=0x19 rs=0 rt=8 imm=5
  const auto daddiu_word = mips::EncodeI(/*opcode=*/0x19U, /*rs=*/0U, /*rt=*/8U,
                                         /*imm16=*/5);
  mips::test::ExecuteOne<kArch>(lifter, "mips64_daddiu_t0_zero_5",
                               mips::Bytes32(daddiu_word), addr, &st, &mem);
  EXPECT_EQ(st.gpr.t0.qword, 5u);

  st.gpr.t1.qword = 7u;

  // daddu $t2, $t0, $t1 => funct=0x2D
  const auto daddu_word = mips::EncodeR(/*rs=*/8, /*rt=*/9, /*rd=*/10,
                                        /*shamt=*/0, /*funct=*/0x2D);
  st.pc.qword = addr + 4;
  mips::test::ExecuteOne<kArch>(lifter, "mips64_daddu_t2_t0_t1",
                               mips::Bytes32(daddu_word), addr + 4, &st, &mem);
  EXPECT_EQ(st.gpr.t2.qword, 12u);

  // dsubu $t3, $t2, $t1 => funct=0x2F
  const auto dsubu_word = mips::EncodeR(/*rs=*/10, /*rt=*/9, /*rd=*/11,
                                        /*shamt=*/0, /*funct=*/0x2F);
  st.pc.qword = addr + 8;
  mips::test::ExecuteOne<kArch>(lifter, "mips64_dsubu_t3_t2_t1",
                               mips::Bytes32(dsubu_word), addr + 8, &st, &mem);
  EXPECT_EQ(st.gpr.t3.qword, 5u);
}

TEST(MIPS64EL, LoadStoreDoubleword_Ld_Sd) {
  llvm::LLVMContext context;
  test_runner::LiftingTester lifter(context, remill::OSName::kOSLinux, kArch);

  const uint64_t addr = 0x3000;
  const uint64_t mem_addr = 0x10000;

  std::unordered_map<uint64_t, uint8_t> init;
  test_runner::MemoryHandler mem(llvm::endianness::little, std::move(init));
  mem.WriteMemory<uint64_t>(mem_addr, 0x1122'3344'5566'7788ULL);

  MIPSState st = {};
  st.pc.qword = addr;
  st.gpr.t1.qword = mem_addr;

  // ld $t0, 0($t1) => opcode=55 (0x37) base=t1 rt=t0
  const auto ld_word = mips::EncodeI(/*opcode=*/0x37U, /*rs=*/9U, /*rt=*/8U,
                                     /*imm16=*/0);
  mips::test::ExecuteOne<kArch>(lifter, "mips64_ld_t0_0_t1",
                               mips::Bytes32(ld_word), addr, &st, &mem);
  EXPECT_EQ(st.gpr.t0.qword, 0x1122'3344'5566'7788ULL);

  // sd $t0, 8($t1) => opcode=63 (0x3F) base=t1 rt=t0
  const auto sd_word = mips::EncodeI(/*opcode=*/0x3FU, /*rs=*/9U, /*rt=*/8U,
                                     /*imm16=*/8);
  st.pc.qword = addr + 4;
  mips::test::ExecuteOne<kArch>(lifter, "mips64_sd_t0_8_t1",
                               mips::Bytes32(sd_word), addr + 4, &st, &mem);
  EXPECT_EQ(mem.ReadMemory<uint64_t>(mem_addr + 8),
            0x1122'3344'5566'7788ULL);
}

TEST(MIPS64EL, Syscall_Break_TrapHypercalls) {
  llvm::LLVMContext context;
  test_runner::LiftingTester lifter(context, remill::OSName::kOSLinux, kArch);

  test_runner::MemoryHandler mem(llvm::endianness::little);
  MIPSState st = {};

  const uint64_t addr = 0x4000;
  st.pc.qword = addr;
  st.hyper_call = AsyncHyperCall::kInvalid;

  // syscall => 0x0000000C
  mips::test::ExecuteOne<kArch>(lifter, "mips64_syscall", mips::Bytes32(0x0CU),
                               addr, &st, &mem);
  EXPECT_EQ(st.hyper_call, AsyncHyperCall::kMIPSSysCall);

  // break => 0x0000000D
  st.pc.qword = addr + 4;
  st.hyper_call = AsyncHyperCall::kInvalid;
  mips::test::ExecuteOne<kArch>(lifter, "mips64_break", mips::Bytes32(0x0DU),
                               addr + 4, &st, &mem);
  EXPECT_EQ(st.hyper_call, AsyncHyperCall::kMIPSBreak);

  // teq $t0, $t1 (trap if equal) => funct=0x34
  const auto teq_word = mips::EncodeR(/*rs=*/8, /*rt=*/9, /*rd=*/0,
                                      /*shamt=*/0, /*funct=*/0x34);
  st.gpr.t0.qword = 5u;
  st.gpr.t1.qword = 5u;
  st.pc.qword = addr + 8;
  st.hyper_call = AsyncHyperCall::kInvalid;
  mips::test::ExecuteOne<kArch>(lifter, "mips64_teq_t0_t1",
                               mips::Bytes32(teq_word), addr + 8, &st, &mem);
  EXPECT_EQ(st.hyper_call, AsyncHyperCall::kMIPSTrap);
}

TEST(MIPS64EL, DelaySlot_Beq_Taken) {
  const uint64_t base = 0x50000;

  const auto beq_word = mips::EncodeI(/*opcode=*/0x04U, /*rs=*/8U, /*rt=*/8U,
                                      /*imm16=*/2);
  const auto ds_word =
      mips::EncodeI(/*opcode=*/0x19U, /*rs=*/0U, /*rt=*/9U, /*imm16=*/1);
  const auto fallthrough_word =
      mips::EncodeI(/*opcode=*/0x19U, /*rs=*/0U, /*rt=*/9U, /*imm16=*/0xFF);
  const auto target_word =
      mips::EncodeI(/*opcode=*/0x19U, /*rs=*/9U, /*rt=*/9U, /*imm16=*/2);

  std::unordered_map<uint64_t, uint8_t> bytes;
  mips::WriteWord32LE(bytes, base + 0x0, beq_word);
  mips::WriteWord32LE(bytes, base + 0x4, ds_word);
  mips::WriteWord32LE(bytes, base + 0x8, fallthrough_word);
  mips::WriteWord32LE(bytes, base + 0xC, target_word);
  // TraceLifter reads up to `arch->MaxInstructionSize()` bytes per
  // instruction. Provide one extra word so it can decode the final instruction.
  mips::WriteWord32LE(bytes, base + 0x10, 0x0u);

  test_runner::MemoryHandler mem(llvm::endianness::little);
  MIPSState st = {};
  st.pc.qword = base;
  st.gpr.t0.qword = 1u;
  st.gpr.t1.qword = 0u;

  mips::test::ExecuteTrace<kArch>("mips64_trace_beq_taken", std::move(bytes),
                                  base, &st, &mem);

  EXPECT_EQ(st.gpr.t1.qword, 3u);
}

TEST(MIPS64EL, DelaySlot_Beql_NotTaken_Annulled) {
  const uint64_t base = 0x60000;

  // Layout:
  //   base+0x0:  beql t0,t1, +4      ; not taken, delay slot annulled; target=base+0x14
  //   base+0x4:  daddiu t3,zero,1    ; delay slot (should be skipped)
  //   base+0x8:  daddiu t2,zero,5    ; fallthrough (executes)
  //   base+0xC:  j    base+0x1C      ; jump around target
  //   base+0x10: nop                 ; delay slot of `j`
  //   base+0x14: daddiu t2,zero,9    ; taken target (should be skipped)
  const auto beql_word = mips::EncodeI(/*opcode=*/0x14U, /*rs=*/8U, /*rt=*/9U,
                                       /*imm16=*/4);
  const auto ds_word =
      mips::EncodeI(/*opcode=*/0x19U, /*rs=*/0U, /*rt=*/11U, /*imm16=*/1);
  const auto fallthrough_word =
      mips::EncodeI(/*opcode=*/0x19U, /*rs=*/0U, /*rt=*/10U, /*imm16=*/5);
  const auto jump_around_word = mips::EncodeJ(/*opcode=*/0x02U, base + 0x1C);
  const auto jump_delay_slot_word = 0x0U;
  const auto target_word =
      mips::EncodeI(/*opcode=*/0x19U, /*rs=*/0U, /*rt=*/10U, /*imm16=*/9);

  std::unordered_map<uint64_t, uint8_t> bytes;
  mips::WriteWord32LE(bytes, base + 0x0, beql_word);
  mips::WriteWord32LE(bytes, base + 0x4, ds_word);
  mips::WriteWord32LE(bytes, base + 0x8, fallthrough_word);
  mips::WriteWord32LE(bytes, base + 0xC, jump_around_word);
  mips::WriteWord32LE(bytes, base + 0x10, jump_delay_slot_word);
  mips::WriteWord32LE(bytes, base + 0x14, target_word);
  // TraceLifter reads up to `arch->MaxInstructionSize()` bytes per instruction.
  // Provide one extra word so it can decode the final instruction.
  mips::WriteWord32LE(bytes, base + 0x18, 0x0u);

  test_runner::MemoryHandler mem(llvm::endianness::little);
  MIPSState st = {};
  st.pc.qword = base;
  st.gpr.t0.qword = 1u;
  st.gpr.t1.qword = 2u;
  st.gpr.t2.qword = 0u;
  st.gpr.t3.qword = 0u;

  mips::test::ExecuteTrace<kArch>("mips64_trace_beql_not_taken",
                                  std::move(bytes), base, &st, &mem);
  EXPECT_EQ(st.gpr.t3.qword, 0u);
  EXPECT_EQ(st.gpr.t2.qword, 5u);
}

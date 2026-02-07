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
    remill::ArchName::kArchMIPS32LittleEndian;
}  // namespace

TEST(MIPS32EL, ZeroWriteIsIgnored_Addiu) {
  llvm::LLVMContext context;
  test_runner::LiftingTester lifter(context, remill::OSName::kOSLinux, kArch);

  const uint64_t addr = 0x1000;

  // addiu $zero, $zero, 1  => opcode=0x09 rs=0 rt=0 imm=1
  const auto word = mips::EncodeI(/*opcode=*/0x09U, /*rs=*/0U, /*rt=*/0U,
                                  /*imm16=*/1);

  MIPSState st = {};
  st.pc.dword = static_cast<uint32_t>(addr);
  st.gpr.zero.dword = 0xDEADBEEFu;

  test_runner::MemoryHandler mem(llvm::endianness::little);
  mips::test::ExecuteOne<kArch>(lifter, "mips32_addiu_zero_zero_1",
                               mips::Bytes32(word), addr, &st, &mem);

  EXPECT_EQ(st.gpr.zero.dword, 0u);
  EXPECT_EQ(st.pc.dword, static_cast<uint32_t>(addr + 4));
}

TEST(MIPS32EL, ZeroReadAsZero_Addiu) {
  llvm::LLVMContext context;
  test_runner::LiftingTester lifter(context, remill::OSName::kOSLinux, kArch);

  const uint64_t addr = 0x2000;

  // addiu $t0, $zero, 5  => opcode=0x09 rs=0 rt=8 imm=5
  const auto word = mips::EncodeI(/*opcode=*/0x09U, /*rs=*/0U, /*rt=*/8U,
                                  /*imm16=*/5);

  MIPSState st = {};
  st.pc.dword = static_cast<uint32_t>(addr);
  st.gpr.zero.dword = 0xCAFEBABEu;
  st.gpr.t0.dword = 0u;

  test_runner::MemoryHandler mem(llvm::endianness::little);
  mips::test::ExecuteOne<kArch>(lifter, "mips32_addiu_t0_zero_5",
                               mips::Bytes32(word), addr, &st, &mem);

  EXPECT_EQ(st.gpr.zero.dword, 0u);
  EXPECT_EQ(st.gpr.t0.dword, 5u);
}

TEST(MIPS32EL, Add_Sub_And_Or_Xor) {
  llvm::LLVMContext context;
  test_runner::LiftingTester lifter(context, remill::OSName::kOSLinux, kArch);

  const uint64_t addr = 0x3000;

  MIPSState st = {};
  st.pc.dword = static_cast<uint32_t>(addr);
  st.gpr.t0.dword = 1u;
  st.gpr.t1.dword = 2u;

  test_runner::MemoryHandler mem(llvm::endianness::little);

  // addu $t2, $t0, $t1  => funct=0x21
  const auto addu_word = mips::EncodeR(/*rs=*/8, /*rt=*/9, /*rd=*/10,
                                       /*shamt=*/0, /*funct=*/0x21);
  mips::test::ExecuteOne<kArch>(lifter, "mips32_addu_t2_t0_t1",
                               mips::Bytes32(addu_word), addr, &st, &mem);
  EXPECT_EQ(st.gpr.t2.dword, 3u);

  // subu $t3, $t1, $t0  => funct=0x23
  const auto subu_word = mips::EncodeR(/*rs=*/9, /*rt=*/8, /*rd=*/11,
                                       /*shamt=*/0, /*funct=*/0x23);
  st.pc.dword = static_cast<uint32_t>(addr + 4);
  mips::test::ExecuteOne<kArch>(lifter, "mips32_subu_t3_t1_t0",
                               mips::Bytes32(subu_word), addr + 4, &st, &mem);
  EXPECT_EQ(st.gpr.t3.dword, 1u);

  // and $t4, $t2, $t3 => funct=0x24
  const auto and_word = mips::EncodeR(/*rs=*/10, /*rt=*/11, /*rd=*/12,
                                      /*shamt=*/0, /*funct=*/0x24);
  st.pc.dword = static_cast<uint32_t>(addr + 8);
  mips::test::ExecuteOne<kArch>(lifter, "mips32_and_t4_t2_t3",
                               mips::Bytes32(and_word), addr + 8, &st, &mem);
  EXPECT_EQ(st.gpr.t4.dword, 1u);

  // or $t5, $t2, $t3 => funct=0x25
  const auto or_word = mips::EncodeR(/*rs=*/10, /*rt=*/11, /*rd=*/13,
                                     /*shamt=*/0, /*funct=*/0x25);
  st.pc.dword = static_cast<uint32_t>(addr + 12);
  mips::test::ExecuteOne<kArch>(lifter, "mips32_or_t5_t2_t3",
                               mips::Bytes32(or_word), addr + 12, &st, &mem);
  EXPECT_EQ(st.gpr.t5.dword, 3u);

  // xor $t6, $t2, $t3 => funct=0x26
  const auto xor_word = mips::EncodeR(/*rs=*/10, /*rt=*/11, /*rd=*/14,
                                      /*shamt=*/0, /*funct=*/0x26);
  st.pc.dword = static_cast<uint32_t>(addr + 16);
  mips::test::ExecuteOne<kArch>(lifter, "mips32_xor_t6_t2_t3",
                               mips::Bytes32(xor_word), addr + 16, &st, &mem);
  EXPECT_EQ(st.gpr.t6.dword, 2u);
}

TEST(MIPS32EL, ShiftImmediate_SrlVsSra) {
  llvm::LLVMContext context;
  test_runner::LiftingTester lifter(context, remill::OSName::kOSLinux, kArch);

  const uint64_t addr = 0x4000;
  MIPSState st = {};
  st.pc.dword = static_cast<uint32_t>(addr);
  st.gpr.t1.dword = 0x8000'0000u;

  test_runner::MemoryHandler mem(llvm::endianness::little);

  // srl $t0, $t1, 1 => rs=0 rt=t1 rd=t0 shamt=1 funct=0x02
  const auto srl_word = mips::EncodeR(/*rs=*/0, /*rt=*/9, /*rd=*/8,
                                      /*shamt=*/1, /*funct=*/0x02);
  mips::test::ExecuteOne<kArch>(lifter, "mips32_srl_t0_t1_1",
                               mips::Bytes32(srl_word), addr, &st, &mem);
  EXPECT_EQ(st.gpr.t0.dword, 0x4000'0000u);

  // sra $t2, $t1, 1 => rs=0 rt=t1 rd=t2 shamt=1 funct=0x03
  const auto sra_word = mips::EncodeR(/*rs=*/0, /*rt=*/9, /*rd=*/10,
                                      /*shamt=*/1, /*funct=*/0x03);
  st.pc.dword = static_cast<uint32_t>(addr + 4);
  mips::test::ExecuteOne<kArch>(lifter, "mips32_sra_t2_t1_1",
                               mips::Bytes32(sra_word), addr + 4, &st, &mem);
  EXPECT_EQ(st.gpr.t2.dword, 0xC000'0000u);
}

TEST(MIPS32EL, LoadSignAndZeroExtension_Lb_Lbu) {
  llvm::LLVMContext context;
  test_runner::LiftingTester lifter(context, remill::OSName::kOSLinux, kArch);

  const uint64_t addr = 0x5000;
  const uint64_t mem_addr = 0x10000;

  std::unordered_map<uint64_t, uint8_t> init = {{mem_addr, 0x80u}};
  test_runner::MemoryHandler mem(llvm::endianness::little, std::move(init));

  MIPSState st = {};
  st.pc.dword = static_cast<uint32_t>(addr);
  st.gpr.t1.dword = static_cast<uint32_t>(mem_addr);

  // lb $t0, 0($t1) => opcode=0x20 base=t1 rt=t0
  const auto lb_word = mips::EncodeI(/*opcode=*/0x20U, /*rs=*/9U, /*rt=*/8U,
                                     /*imm16=*/0);
  mips::test::ExecuteOne<kArch>(lifter, "mips32_lb_t0_0_t1",
                               mips::Bytes32(lb_word), addr, &st, &mem);
  EXPECT_EQ(st.gpr.t0.dword, 0xFFFF'FF80u);

  // lbu $t2, 0($t1) => opcode=0x24 base=t1 rt=t2
  const auto lbu_word = mips::EncodeI(/*opcode=*/0x24U, /*rs=*/9U, /*rt=*/10U,
                                      /*imm16=*/0);
  st.pc.dword = static_cast<uint32_t>(addr + 4);
  mips::test::ExecuteOne<kArch>(lifter, "mips32_lbu_t2_0_t1",
                               mips::Bytes32(lbu_word), addr + 4, &st, &mem);
  EXPECT_EQ(st.gpr.t2.dword, 0x0000'0080u);
}

TEST(MIPS32EL, StoreWord_Sw) {
  llvm::LLVMContext context;
  test_runner::LiftingTester lifter(context, remill::OSName::kOSLinux, kArch);

  const uint64_t addr = 0x6000;
  const uint64_t mem_addr = 0x20000;

  test_runner::MemoryHandler mem(llvm::endianness::little);

  MIPSState st = {};
  st.pc.dword = static_cast<uint32_t>(addr);
  st.gpr.t0.dword = 0x1122'3344u;
  st.gpr.t1.dword = static_cast<uint32_t>(mem_addr);

  // sw $t0, 0($t1) => opcode=0x2B base=t1 rt=t0
  const auto sw_word = mips::EncodeI(/*opcode=*/0x2BU, /*rs=*/9U, /*rt=*/8U,
                                     /*imm16=*/0);
  mips::test::ExecuteOne<kArch>(lifter, "mips32_sw_t0_0_t1",
                               mips::Bytes32(sw_word), addr, &st, &mem);

  EXPECT_EQ(mem.ReadMemory<uint32_t>(mem_addr), 0x1122'3344u);
}

TEST(MIPS32EL, Mult_Div_HiLo) {
  llvm::LLVMContext context;
  test_runner::LiftingTester lifter(context, remill::OSName::kOSLinux, kArch);

  const uint64_t addr = 0x7000;
  test_runner::MemoryHandler mem(llvm::endianness::little);

  MIPSState st = {};
  st.pc.dword = static_cast<uint32_t>(addr);
  st.gpr.t0.dword = 10u;
  st.gpr.t1.dword = 3u;

  // mult $t0, $t1 => funct=0x18
  const auto mult_word = mips::EncodeR(/*rs=*/8, /*rt=*/9, /*rd=*/0,
                                       /*shamt=*/0, /*funct=*/0x18);
  mips::test::ExecuteOne<kArch>(lifter, "mips32_mult_t0_t1",
                               mips::Bytes32(mult_word), addr, &st, &mem);
  EXPECT_EQ(st.lo.dword, 30u);
  EXPECT_EQ(st.hi.dword, 0u);

  // div $t0, $t1 => funct=0x1A
  const auto div_word = mips::EncodeR(/*rs=*/8, /*rt=*/9, /*rd=*/0,
                                      /*shamt=*/0, /*funct=*/0x1A);
  st.pc.dword = static_cast<uint32_t>(addr + 4);
  mips::test::ExecuteOne<kArch>(lifter, "mips32_div_t0_t1",
                               mips::Bytes32(div_word), addr + 4, &st, &mem);
  EXPECT_EQ(st.lo.dword, 3u);
  EXPECT_EQ(st.hi.dword, 1u);
}

TEST(MIPS32EL, Syscall_Break_TrapHypercalls) {
  llvm::LLVMContext context;
  test_runner::LiftingTester lifter(context, remill::OSName::kOSLinux, kArch);

  test_runner::MemoryHandler mem(llvm::endianness::little);
  MIPSState st = {};

  const uint64_t addr = 0x8000;
  st.pc.dword = static_cast<uint32_t>(addr);
  st.hyper_call = AsyncHyperCall::kInvalid;

  // syscall => 0x0000000C
  mips::test::ExecuteOne<kArch>(lifter, "mips32_syscall", mips::Bytes32(0x0CU),
                               addr, &st, &mem);
  EXPECT_EQ(st.hyper_call, AsyncHyperCall::kMIPSSysCall);

  // break => 0x0000000D (modeled as break hypercall via trap pcodeop mapping).
  st.pc.dword = static_cast<uint32_t>(addr + 4);
  st.hyper_call = AsyncHyperCall::kInvalid;
  mips::test::ExecuteOne<kArch>(lifter, "mips32_break", mips::Bytes32(0x0DU),
                               addr + 4, &st, &mem);
  EXPECT_EQ(st.hyper_call, AsyncHyperCall::kMIPSBreak);

  // teq $t0, $t1 (trap if equal) => funct=0x34
  const auto teq_word = mips::EncodeR(/*rs=*/8, /*rt=*/9, /*rd=*/0,
                                      /*shamt=*/0, /*funct=*/0x34);
  st.gpr.t0.dword = 5u;
  st.gpr.t1.dword = 5u;
  st.pc.dword = static_cast<uint32_t>(addr + 8);
  st.hyper_call = AsyncHyperCall::kInvalid;
  mips::test::ExecuteOne<kArch>(lifter, "mips32_teq_t0_t1",
                               mips::Bytes32(teq_word), addr + 8, &st, &mem);
  EXPECT_EQ(st.hyper_call, AsyncHyperCall::kMIPSTrap);
}

TEST(MIPS32EL, DelaySlot_Beq_Taken) {
  const uint64_t base = 0x10000;

  // Layout:
  //   base+0x0:  beq  t0,t0, +2      ; target=base+0xC
  //   base+0x4:  addiu t1,zero,1     ; delay slot (always executes)
  //   base+0x8:  addiu t1,zero,0xFF  ; fallthrough (should be skipped)
  //   base+0xC:  addiu t1,t1,2       ; branch target
  const auto beq_word = mips::EncodeI(/*opcode=*/0x04U, /*rs=*/8U, /*rt=*/8U,
                                      /*imm16=*/2);
  const auto ds_word =
      mips::EncodeI(/*opcode=*/0x09U, /*rs=*/0U, /*rt=*/9U, /*imm16=*/1);
  const auto fallthrough_word =
      mips::EncodeI(/*opcode=*/0x09U, /*rs=*/0U, /*rt=*/9U, /*imm16=*/0xFF);
  const auto target_word =
      mips::EncodeI(/*opcode=*/0x09U, /*rs=*/9U, /*rt=*/9U, /*imm16=*/2);

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
  st.pc.dword = static_cast<uint32_t>(base);
  st.gpr.t0.dword = 1u;
  st.gpr.t1.dword = 0u;

  mips::test::ExecuteTrace<kArch>("mips32_trace_beq_taken", std::move(bytes),
                                  base, &st, &mem);

  // Delay slot executes (t1=1), and taken target executes (t1=3). The
  // fallthrough instruction at base+0x8 should not execute.
  EXPECT_EQ(st.gpr.t1.dword, 3u);
}

TEST(MIPS32EL, DelaySlot_Beql_NotTaken_Annulled) {
  const uint64_t base = 0x20000;

  // Layout:
  //   base+0x0:  beql t0,t1, +4      ; not taken, delay slot annulled; target=base+0x14
  //   base+0x4:  addiu t3,zero,1     ; delay slot (should be skipped)
  //   base+0x8:  addiu t2,zero,5     ; fallthrough (executes)
  //   base+0xC:  j    base+0x1C      ; jump around target
  //   base+0x10: nop                 ; delay slot of `j`
  //   base+0x14: addiu t2,zero,9     ; taken target (should be skipped)
  const auto beql_word = mips::EncodeI(/*opcode=*/0x14U, /*rs=*/8U, /*rt=*/9U,
                                       /*imm16=*/4);
  const auto ds_word =
      mips::EncodeI(/*opcode=*/0x09U, /*rs=*/0U, /*rt=*/11U, /*imm16=*/1);
  const auto fallthrough_word =
      mips::EncodeI(/*opcode=*/0x09U, /*rs=*/0U, /*rt=*/10U, /*imm16=*/5);
  const auto jump_around_word = mips::EncodeJ(/*opcode=*/0x02U, base + 0x1C);
  const auto jump_delay_slot_word = 0x0U;
  const auto target_word =
      mips::EncodeI(/*opcode=*/0x09U, /*rs=*/0U, /*rt=*/10U, /*imm16=*/9);

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
  st.pc.dword = static_cast<uint32_t>(base);
  st.gpr.t0.dword = 1u;
  st.gpr.t1.dword = 2u;  // not equal -> not taken
  st.gpr.t2.dword = 0u;
  st.gpr.t3.dword = 0u;

  mips::test::ExecuteTrace<kArch>("mips32_trace_beql_not_taken",
                                  std::move(bytes), base, &st, &mem);

  // Not taken path should skip the delay slot and execute the fallthrough,
  // then jump around the taken target.
  EXPECT_EQ(st.gpr.t3.dword, 0u);
  EXPECT_EQ(st.gpr.t2.dword, 5u);
}

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

#include "Arch.h"

#include <glog/logging.h>
#include <remill/Arch/Name.h>
#include <remill/BC/ABI.h>
#include <remill/BC/Util.h>

#include <string>
#include <utility>

#define INCLUDED_FROM_REMILL
#include <remill/Arch/MIPS/Runtime/State.h>

namespace remill {
namespace sleighmips {

class SleighMIPSDecoder : public sleigh::SleighDecoder {
 public:
  SleighMIPSDecoder(const remill::Arch &arch, std::string sla_name,
                    std::string pspec_name)
      : SleighDecoder(arch, std::move(sla_name), std::move(pspec_name),
                      sleigh::ContextRegMappings({}, {}),
                      /*state_reg_remappings=*/{}) {}

  void InitializeSleighContext(uint64_t address,
                               sleigh::SingleInstructionSleighContext &ctxt,
                               const ContextValues &) const override {
    const auto sleigh_addr = ctxt.GetAddressFromOffset(address);

    // Force "regular" MIPS decoding (not mips16/micromips).
    ctxt.GetContext().setVariable("ISA_MODE", sleigh_addr, 0);
    ctxt.GetContext().setVariable("LowBitCodeMode", sleigh_addr, 0);

    // Disable paired-instruction decoding state for LWL/LWR.
    ctxt.GetContext().setVariable("PAIR_INSTRUCTION_FLAG", sleigh_addr, 0);
  }

  llvm::Value *LiftPcFromCurrPc(llvm::IRBuilder<> &bldr, llvm::Value *curr_pc,
                                size_t curr_insn_size,
                                const DecodingContext &) const override {
    return bldr.CreateAdd(
        curr_pc, llvm::ConstantInt::get(curr_pc->getType(), curr_insn_size));
  }
};

class SleighMIPSArch : public ArchBase {
 public:
  SleighMIPSArch(llvm::LLVMContext *context_, OSName os_name_,
                 ArchName arch_name_, std::string sla_name,
                 std::string pspec_name)
      : ArchBase(context_, os_name_, arch_name_),
        decoder(*this, std::move(sla_name), std::move(pspec_name)) {}

  DecodingContext CreateInitialContext(void) const override {
    return DecodingContext();
  }

  std::string_view StackPointerRegisterName(void) const override {
    return "SP";
  }

  std::string_view ProgramCounterRegisterName(void) const override {
    return "PC";
  }

  OperandLifter::OpLifterPtr
  DefaultLifter(const remill::IntrinsicTable &) const override {
    return decoder.GetOpLifter();
  }

  bool DecodeInstruction(uint64_t address, std::string_view instr_bytes,
                         Instruction &inst,
                         DecodingContext context) const override {
    return decoder.DecodeInstruction(address, instr_bytes, inst, context);
  }

  uint64_t MinInstructionAlign(const DecodingContext &) const override {
    return 4;
  }

  uint64_t MinInstructionSize(const DecodingContext &) const override {
    return 4;
  }

  uint64_t MaxInstructionSize(const DecodingContext &, bool) const override {
    // Conditional branches and calls in the Ghidra MIPS Sleigh specification
    // model delay slot semantics using `delayslot(1)`, which causes Sleigh's
    // decoder to consume and lift the delay slot instruction as part of the
    // control-flow instruction.
    return 8;
  }

  llvm::CallingConv::ID DefaultCallingConv(void) const override {
    return llvm::CallingConv::C;
  }

  llvm::Triple Triple(void) const override {
    auto triple = BasicTriple();
    switch (arch_name) {
      case kArchMIPS32LittleEndian: triple.setArch(llvm::Triple::mipsel); break;
      case kArchMIPS64LittleEndian:
        triple.setArch(llvm::Triple::mips64el);
        break;
      default:
        LOG(FATAL) << "Cannot get triple for non-MIPS architecture "
                   << GetArchName(arch_name);
        break;
    }
    return triple;
  }

  llvm::DataLayout DataLayout(void) const override {
    switch (arch_name) {
      case kArchMIPS32LittleEndian:
        return llvm::DataLayout("e-m:m-p:32:32-i8:8:32-i16:16:32-i64:64-n32-S64");
      case kArchMIPS64LittleEndian:
        return llvm::DataLayout("e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128");
      default:
        LOG(FATAL) << "Cannot get data layout for non-MIPS architecture "
                   << GetArchName(arch_name);
        return llvm::DataLayout("");
    }
  }

  void PopulateRegisterTable(void) const override {
    CHECK_NOTNULL(context);

    reg_by_offset.resize(sizeof(MIPSState));

    auto u8 = llvm::Type::getInt8Ty(*context);
    auto u32 = llvm::Type::getInt32Ty(*context);
    auto u64 = llvm::Type::getInt64Ty(*context);

#define OFFSET_OF(state, access) \
  (reinterpret_cast<uintptr_t>(&state.access) - reinterpret_cast<uintptr_t>(&state))

#define REG(state, name, offset, type) \
  AddRegister(#name, type, offset, nullptr)

    MIPSState state;

    if (arch_name == kArchMIPS32LittleEndian) {
      REG(state, ZERO, OFFSET_OF(state, gpr.zero.dword), u32);
      REG(state, AT, OFFSET_OF(state, gpr.at.dword), u32);
      REG(state, V0, OFFSET_OF(state, gpr.v0.dword), u32);
      REG(state, V1, OFFSET_OF(state, gpr.v1.dword), u32);
      REG(state, A0, OFFSET_OF(state, gpr.a0.dword), u32);
      REG(state, A1, OFFSET_OF(state, gpr.a1.dword), u32);
      REG(state, A2, OFFSET_OF(state, gpr.a2.dword), u32);
      REG(state, A3, OFFSET_OF(state, gpr.a3.dword), u32);
      REG(state, T0, OFFSET_OF(state, gpr.t0.dword), u32);
      REG(state, T1, OFFSET_OF(state, gpr.t1.dword), u32);
      REG(state, T2, OFFSET_OF(state, gpr.t2.dword), u32);
      REG(state, T3, OFFSET_OF(state, gpr.t3.dword), u32);
      REG(state, T4, OFFSET_OF(state, gpr.t4.dword), u32);
      REG(state, T5, OFFSET_OF(state, gpr.t5.dword), u32);
      REG(state, T6, OFFSET_OF(state, gpr.t6.dword), u32);
      REG(state, T7, OFFSET_OF(state, gpr.t7.dword), u32);
      REG(state, S0, OFFSET_OF(state, gpr.s0.dword), u32);
      REG(state, S1, OFFSET_OF(state, gpr.s1.dword), u32);
      REG(state, S2, OFFSET_OF(state, gpr.s2.dword), u32);
      REG(state, S3, OFFSET_OF(state, gpr.s3.dword), u32);
      REG(state, S4, OFFSET_OF(state, gpr.s4.dword), u32);
      REG(state, S5, OFFSET_OF(state, gpr.s5.dword), u32);
      REG(state, S6, OFFSET_OF(state, gpr.s6.dword), u32);
      REG(state, S7, OFFSET_OF(state, gpr.s7.dword), u32);
      REG(state, T8, OFFSET_OF(state, gpr.t8.dword), u32);
      REG(state, T9, OFFSET_OF(state, gpr.t9.dword), u32);
      REG(state, K0, OFFSET_OF(state, gpr.k0.dword), u32);
      REG(state, K1, OFFSET_OF(state, gpr.k1.dword), u32);
      REG(state, GP, OFFSET_OF(state, gpr.gp.dword), u32);
      REG(state, SP, OFFSET_OF(state, gpr.sp.dword), u32);
      REG(state, S8, OFFSET_OF(state, gpr.s8.dword), u32);
      REG(state, RA, OFFSET_OF(state, gpr.ra.dword), u32);

      REG(state, PC, OFFSET_OF(state, pc.dword), u32);
      REG(state, HI, OFFSET_OF(state, hi.dword), u32);
      REG(state, LO, OFFSET_OF(state, lo.dword), u32);
      AddRegister("ACHI", u32, OFFSET_OF(state, hi.dword), "HI");
      AddRegister("ACLO", u32, OFFSET_OF(state, lo.dword), "LO");

    } else {
      REG(state, ZERO, OFFSET_OF(state, gpr.zero.qword), u64);
      REG(state, AT, OFFSET_OF(state, gpr.at.qword), u64);
      REG(state, V0, OFFSET_OF(state, gpr.v0.qword), u64);
      REG(state, V1, OFFSET_OF(state, gpr.v1.qword), u64);
      REG(state, A0, OFFSET_OF(state, gpr.a0.qword), u64);
      REG(state, A1, OFFSET_OF(state, gpr.a1.qword), u64);
      REG(state, A2, OFFSET_OF(state, gpr.a2.qword), u64);
      REG(state, A3, OFFSET_OF(state, gpr.a3.qword), u64);
      REG(state, T0, OFFSET_OF(state, gpr.t0.qword), u64);
      REG(state, T1, OFFSET_OF(state, gpr.t1.qword), u64);
      REG(state, T2, OFFSET_OF(state, gpr.t2.qword), u64);
      REG(state, T3, OFFSET_OF(state, gpr.t3.qword), u64);
      REG(state, T4, OFFSET_OF(state, gpr.t4.qword), u64);
      REG(state, T5, OFFSET_OF(state, gpr.t5.qword), u64);
      REG(state, T6, OFFSET_OF(state, gpr.t6.qword), u64);
      REG(state, T7, OFFSET_OF(state, gpr.t7.qword), u64);
      REG(state, S0, OFFSET_OF(state, gpr.s0.qword), u64);
      REG(state, S1, OFFSET_OF(state, gpr.s1.qword), u64);
      REG(state, S2, OFFSET_OF(state, gpr.s2.qword), u64);
      REG(state, S3, OFFSET_OF(state, gpr.s3.qword), u64);
      REG(state, S4, OFFSET_OF(state, gpr.s4.qword), u64);
      REG(state, S5, OFFSET_OF(state, gpr.s5.qword), u64);
      REG(state, S6, OFFSET_OF(state, gpr.s6.qword), u64);
      REG(state, S7, OFFSET_OF(state, gpr.s7.qword), u64);
      REG(state, T8, OFFSET_OF(state, gpr.t8.qword), u64);
      REG(state, T9, OFFSET_OF(state, gpr.t9.qword), u64);
      REG(state, K0, OFFSET_OF(state, gpr.k0.qword), u64);
      REG(state, K1, OFFSET_OF(state, gpr.k1.qword), u64);
      REG(state, GP, OFFSET_OF(state, gpr.gp.qword), u64);
      REG(state, SP, OFFSET_OF(state, gpr.sp.qword), u64);
      REG(state, S8, OFFSET_OF(state, gpr.s8.qword), u64);
      REG(state, RA, OFFSET_OF(state, gpr.ra.qword), u64);

      REG(state, PC, OFFSET_OF(state, pc.qword), u64);
      REG(state, HI, OFFSET_OF(state, hi.qword), u64);
      REG(state, LO, OFFSET_OF(state, lo.qword), u64);
      AddRegister("ACHI", u64, OFFSET_OF(state, hi.qword), "HI");
      AddRegister("ACLO", u64, OFFSET_OF(state, lo.qword), "LO");
    }

    const auto fpr_base = OFFSET_OF(state, fpr.bytes);
    const auto fpr_file = llvm::ArrayType::get(u8, kNumFprBytes);
    AddRegister("FPR", fpr_file, fpr_base, nullptr);

    if (arch_name == kArchMIPS32LittleEndian) {
      // MIPS32 floating-point registers are modeled as 32-bit lanes in the
      // state, but many instructions treat adjacent register pairs as 64-bit
      // values (e.g. double-precision operations). Model this using an
      // explicit parent (F0_1) and two 32-bit subregisters (F0, F1) that
      // overlap.
      for (uint32_t i = 0; i < 32; i += 2) {
        std::stringstream pair_ss;
        pair_ss << "F" << i << "_" << (i + 1u);
        const auto pair_name = pair_ss.str();
        AddRegister(pair_name.c_str(), u64, fpr_base + (i * 4u), "FPR");

        std::stringstream lo_ss;
        lo_ss << "F" << i;
        const auto lo_name = lo_ss.str();
        AddRegister(lo_name.c_str(), u32, fpr_base + (i * 4u),
                    pair_name.c_str());

        std::stringstream hi_ss;
        hi_ss << "F" << (i + 1u);
        const auto hi_name = hi_ss.str();
        AddRegister(hi_name.c_str(), u32, fpr_base + ((i + 1u) * 4u),
                    pair_name.c_str());
      }

    } else {
      for (uint32_t i = 0; i < 32; ++i) {
        std::stringstream ss;
        ss << "F" << i;
        AddRegister(ss.str().c_str(), u64, fpr_base + (i * 8u), "FPR");
      }
    }

    REG(state, FIR, OFFSET_OF(state, fcr.fir), u32);
    REG(state, FCCR, OFFSET_OF(state, fcr.fccr), u32);
    REG(state, FEXR, OFFSET_OF(state, fcr.fexr), u32);
    REG(state, FENR, OFFSET_OF(state, fcr.fenr), u32);
    REG(state, FCSR, OFFSET_OF(state, fcr.fcsr), u32);

#undef REG
#undef OFFSET_OF
  }

  void FinishLiftedFunctionInitialization(llvm::Module *module,
                                         llvm::Function *bb_func) const override {
    auto &context = module->getContext();
    const auto addr = llvm::Type::getIntNTy(context, address_size);

    auto &entry_block = bb_func->getEntryBlock();
    llvm::IRBuilder<> ir(&entry_block);

    const auto pc_arg = NthArgument(bb_func, kPCArgNum);
    const auto state_ptr_arg = NthArgument(bb_func, kStatePointerArgNum);

    auto mk_alloca = [&](auto &from) {
      return ir.CreateAlloca(addr, nullptr, from.data());
    };

    ir.CreateStore(pc_arg, mk_alloca(kNextPCVariableName));
    ir.CreateStore(pc_arg, mk_alloca(kIgnoreNextPCVariableName));

    std::ignore = RegisterByName(kPCVariableName)->AddressOf(state_ptr_arg, ir);
  }

 private:
  SleighMIPSDecoder decoder;
};

}  // namespace sleighmips

Arch::ArchPtr Arch::GetSleighMIPS32EL(llvm::LLVMContext *context_,
                                      remill::OSName os_name_,
                                      remill::ArchName arch_name_) {
  return std::make_unique<sleighmips::SleighMIPSArch>(
      context_, os_name_, arch_name_,
      /*sla_name=*/"mips32le.sla",
      /*pspec_name=*/"mips32.pspec");
}

Arch::ArchPtr Arch::GetSleighMIPS64EL(llvm::LLVMContext *context_,
                                      remill::OSName os_name_,
                                      remill::ArchName arch_name_) {
  return std::make_unique<sleighmips::SleighMIPSArch>(
      context_, os_name_, arch_name_,
      /*sla_name=*/"mips64le.sla",
      /*pspec_name=*/"mips64.pspec");
}

}  // namespace remill

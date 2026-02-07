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

#pragma clang diagnostic push
#pragma clang diagnostic fatal "-Wpadded"

#include "remill/Arch/Runtime/State.h"

#if !defined(INCLUDED_FROM_REMILL)
#  include "remill/Arch/Runtime/Types.h"
#endif

struct Reg final {
  union {
    alignas(4) uint32_t dword;
    alignas(8) uint64_t qword;
  } __attribute__((packed));
} __attribute__((packed));

static_assert(sizeof(uint64_t) == sizeof(Reg), "Invalid packing of `Reg`.");
static_assert(0 == __builtin_offsetof(Reg, dword),
              "Invalid packing of `Reg::dword`.");
static_assert(0 == __builtin_offsetof(Reg, qword),
              "Invalid packing of `Reg::qword`.");

struct alignas(8) GPR final {

  // Prevents LLVM from casting the whole `GPR` into an `i64` to access `ZERO`.
  volatile uint64_t _0;
  Reg zero;
  volatile uint64_t _1;
  Reg at;
  volatile uint64_t _2;
  Reg v0;
  volatile uint64_t _3;
  Reg v1;

  volatile uint64_t _4;
  Reg a0;
  volatile uint64_t _5;
  Reg a1;
  volatile uint64_t _6;
  Reg a2;
  volatile uint64_t _7;
  Reg a3;

  volatile uint64_t _8;
  Reg t0;
  volatile uint64_t _9;
  Reg t1;
  volatile uint64_t _10;
  Reg t2;
  volatile uint64_t _11;
  Reg t3;
  volatile uint64_t _12;
  Reg t4;
  volatile uint64_t _13;
  Reg t5;
  volatile uint64_t _14;
  Reg t6;
  volatile uint64_t _15;
  Reg t7;

  volatile uint64_t _16;
  Reg s0;
  volatile uint64_t _17;
  Reg s1;
  volatile uint64_t _18;
  Reg s2;
  volatile uint64_t _19;
  Reg s3;
  volatile uint64_t _20;
  Reg s4;
  volatile uint64_t _21;
  Reg s5;
  volatile uint64_t _22;
  Reg s6;
  volatile uint64_t _23;
  Reg s7;

  volatile uint64_t _24;
  Reg t8;
  volatile uint64_t _25;
  Reg t9;
  volatile uint64_t _26;
  Reg k0;
  volatile uint64_t _27;
  Reg k1;

  volatile uint64_t _28;
  Reg gp;
  volatile uint64_t _29;
  Reg sp;
  volatile uint64_t _30;
  Reg s8;
  volatile uint64_t _31;
  Reg ra;

} __attribute__((packed));

static_assert(512 == sizeof(GPR), "Invalid structure packing of `GPR`.");

enum : size_t { kNumFprBytes = 32 * 8 };

struct alignas(8) FPR final {
  uint8_t bytes[kNumFprBytes];
} __attribute__((packed));

static_assert(256 == sizeof(FPR), "Invalid structure packing of `FPR`.");

struct alignas(8) FCR final {
  volatile uint32_t _0;
  uint32_t fir;

  volatile uint32_t _1;
  uint32_t fccr;

  volatile uint32_t _2;
  uint32_t fexr;

  volatile uint32_t _3;
  uint32_t fenr;

  volatile uint32_t _4;
  uint32_t fcsr;
} __attribute__((packed));

static_assert(40 == sizeof(FCR), "Invalid structure packing of `FCR`.");

struct alignas(8) MIPSState : public ArchState {
  GPR gpr;
  FPR fpr;

  volatile uint64_t _hi;
  Reg hi;

  volatile uint64_t _lo;
  Reg lo;

  volatile uint64_t _pc;
  Reg pc;

  FCR fcr;
} __attribute__((packed));

struct State : public MIPSState {};

#pragma clang diagnostic pop


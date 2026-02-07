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

#include <cstdint>
#include <string>
#include <unordered_map>

namespace mips {

inline std::string Bytes32(uint32_t word) {
  std::string bytes;
  bytes.resize(4);
  bytes[0] = static_cast<char>(word & 0xFFu);
  bytes[1] = static_cast<char>((word >> 8) & 0xFFu);
  bytes[2] = static_cast<char>((word >> 16) & 0xFFu);
  bytes[3] = static_cast<char>((word >> 24) & 0xFFu);
  return bytes;
}

inline void WriteWord32LE(std::unordered_map<uint64_t, uint8_t> &mem,
                          uint64_t addr, uint32_t word) {
  mem[addr + 0] = static_cast<uint8_t>(word & 0xFFu);
  mem[addr + 1] = static_cast<uint8_t>((word >> 8) & 0xFFu);
  mem[addr + 2] = static_cast<uint8_t>((word >> 16) & 0xFFu);
  mem[addr + 3] = static_cast<uint8_t>((word >> 24) & 0xFFu);
}

constexpr uint32_t EncodeR(uint32_t rs, uint32_t rt, uint32_t rd,
                           uint32_t shamt, uint32_t funct) {
  // opcode=0
  return ((rs & 0x1FU) << 21) | ((rt & 0x1FU) << 16) | ((rd & 0x1FU) << 11) |
         ((shamt & 0x1FU) << 6) | (funct & 0x3FU);
}

constexpr uint32_t EncodeI(uint32_t opcode, uint32_t rs, uint32_t rt,
                           int32_t imm16) {
  const uint32_t imm = static_cast<uint32_t>(imm16) & 0xFFFFU;
  return ((opcode & 0x3FU) << 26) | ((rs & 0x1FU) << 21) | ((rt & 0x1FU) << 16) |
         imm;
}

constexpr uint32_t EncodeJ(uint32_t opcode, uint64_t target_addr) {
  const uint32_t tgt = static_cast<uint32_t>((target_addr >> 2) & 0x03FFFFFFULL);
  return ((opcode & 0x3FU) << 26) | tgt;
}

}  // namespace mips

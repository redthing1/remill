# Copyright (c) 2022 Trail of Bits, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set(REMILL_ARCH_SLEIGH_SOURCES
  "${REMILL_INCLUDE_DIR}/remill/Arch/Runtime/Definitions.h"
  "${REMILL_INCLUDE_DIR}/remill/Arch/Runtime/HyperCall.h"
  "${REMILL_INCLUDE_DIR}/remill/Arch/Runtime/Intrinsics.h"
  "${REMILL_INCLUDE_DIR}/remill/Arch/Runtime/Operators.h"
  "${REMILL_INCLUDE_DIR}/remill/Arch/Runtime/Runtime.h"
  "${REMILL_INCLUDE_DIR}/remill/Arch/Runtime/State.h"
  "${REMILL_INCLUDE_DIR}/remill/Arch/Runtime/Types.h"

  Arch.h
  Arch.cpp
  ControlFlowStructuring.cpp
  ControlFlowStructuring.h
)

if(REMILL_ENABLE_ARCH_X86)
  list(APPEND REMILL_ARCH_SLEIGH_SOURCES
    "${REMILL_INCLUDE_DIR}/remill/Arch/X86/Runtime/Operators.h"
    "${REMILL_INCLUDE_DIR}/remill/Arch/X86/Runtime/State.h"
    "${REMILL_INCLUDE_DIR}/remill/Arch/X86/Runtime/Types.h"
    "${REMILL_INCLUDE_DIR}/remill/Arch/X86/X86Base.h"
    X86Arch.cpp
  )
endif()

if(REMILL_ENABLE_ARCH_AARCH32)
  list(APPEND REMILL_ARCH_SLEIGH_SOURCES
    "${REMILL_INCLUDE_DIR}/remill/Arch/AArch32/Runtime/Operators.h"
    "${REMILL_INCLUDE_DIR}/remill/Arch/AArch32/Runtime/State.h"
    "${REMILL_INCLUDE_DIR}/remill/Arch/AArch32/Runtime/Types.h"
    Thumb.h
    Thumb2Arch.cpp
    AArch32Arch.cpp
    AArch32Arch.h
    ARMBase.cpp
  )
endif()

if(REMILL_ENABLE_ARCH_AARCH64)
  list(APPEND REMILL_ARCH_SLEIGH_SOURCES
    "${REMILL_INCLUDE_DIR}/remill/Arch/AArch64/Runtime/Operators.h"
    "${REMILL_INCLUDE_DIR}/remill/Arch/AArch64/Runtime/State.h"
    "${REMILL_INCLUDE_DIR}/remill/Arch/AArch64/Runtime/Types.h"
    AArch64Arch.cpp
    AArch64Arch.h
  )
endif()

if(REMILL_ENABLE_ARCH_SPARC)
  list(APPEND REMILL_ARCH_SLEIGH_SOURCES
    "${REMILL_INCLUDE_DIR}/remill/Arch/SPARC32/Runtime/State.h"
    "${REMILL_INCLUDE_DIR}/remill/Arch/SPARC32/Runtime/Types.h"
    "${REMILL_INCLUDE_DIR}/remill/Arch/SPARC32/SPARC32Base.h"
    SPARC32Arch.cpp
    SPARC32Arch.h
  )
endif()

if(REMILL_ENABLE_ARCH_PPC)
  list(APPEND REMILL_ARCH_SLEIGH_SOURCES
    "${REMILL_INCLUDE_DIR}/remill/Arch/PPC/Runtime/State.h"
    PPC.h
    PPCArch.cpp
  )
endif()

if(REMILL_ENABLE_ARCH_RISCV)
  list(APPEND REMILL_ARCH_SLEIGH_SOURCES
    "${REMILL_INCLUDE_DIR}/remill/Arch/RISCV/Runtime/State.h"
    RISCVArch.cpp
  )
endif()

if(REMILL_ENABLE_ARCH_MIPS)
  list(APPEND REMILL_ARCH_SLEIGH_SOURCES
    "${REMILL_INCLUDE_DIR}/remill/Arch/MIPS/Runtime/State.h"
    MIPSArch.cpp
  )
endif()

add_library(remill_arch_sleigh STATIC
  ${REMILL_ARCH_SLEIGH_SOURCES}
)

add_dependencies(remill_arch_sleigh sleigh_custom_specs)

set(REMILL_ARCH_SLEIGH_LINK_LIBS remill_settings)

if(REMILL_ENABLE_ARCH_AARCH64)
  list(APPEND REMILL_ARCH_SLEIGH_LINK_LIBS remill_arch_aarch64)
endif()

if(REMILL_ENABLE_ARCH_SPARC)
  list(APPEND REMILL_ARCH_SLEIGH_LINK_LIBS remill_arch_sparc32)
endif()

target_link_libraries(remill_arch_sleigh PUBLIC
  ${REMILL_ARCH_SLEIGH_LINK_LIBS}
)

target_include_directories(remill_arch_sleigh AFTER PRIVATE "${REMILL_SOURCE_DIR}")

if(REMILL_ENABLE_INSTALL_TARGET)
  install(
    TARGETS remill_arch_sleigh
    EXPORT remillTargets
  )
endif()

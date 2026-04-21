# Copyright (c) 2020 Trail of Bits, Inc.
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

add_library(remill_arch STATIC
  "${REMILL_INCLUDE_DIR}/remill/Arch/Arch.h"
  "${REMILL_INCLUDE_DIR}/remill/Arch/Instruction.h"
  "${REMILL_INCLUDE_DIR}/remill/Arch/Name.h"
  "${REMILL_INCLUDE_DIR}/remill/Arch/ArchBase.h"
  "${REMILL_INCLUDE_DIR}/remill/Arch/Context.h"

  Arch.cpp
  BitManipulation.h
  Instruction.cpp
  Context.cpp
  Name.cpp
)

target_link_libraries(remill_arch PUBLIC
  remill_settings
)

set(REMILL_ARCH_LINK_LIBS "")

if(REMILL_ENABLE_ARCH_AARCH32)
  add_subdirectory(AArch32)
endif()

if(REMILL_ENABLE_ARCH_AARCH64)
  add_subdirectory(AArch64)
  list(APPEND REMILL_ARCH_LINK_LIBS remill_arch_aarch64)
endif()

if(REMILL_ENABLE_ARCH_PPC)
  add_subdirectory(PPC)
endif()

if(REMILL_ENABLE_ARCH_MIPS)
  add_subdirectory(MIPS)
endif()

if(REMILL_ENABLE_ARCH_RISCV)
  add_subdirectory(RISCV)
endif()

if(REMILL_ENABLE_ARCH_SPARC)
  add_subdirectory(SPARC32)
  add_subdirectory(SPARC64)
  list(APPEND REMILL_ARCH_LINK_LIBS
    remill_arch_sparc32
    remill_arch_sparc64
  )
endif()

set(REMILL_BUILD_SLEIGH_BACKENDS OFF)
if(REMILL_ENABLE_SLEIGH)
  if(REMILL_ENABLE_ARCH_X86 OR REMILL_ENABLE_ARCH_AARCH64 OR REMILL_ENABLE_ARCH_AARCH32 OR REMILL_ENABLE_ARCH_SPARC OR REMILL_ENABLE_ARCH_PPC OR REMILL_ENABLE_ARCH_RISCV OR REMILL_ENABLE_ARCH_MIPS)
    set(REMILL_BUILD_SLEIGH_BACKENDS ON)
  endif()
endif()

if(REMILL_BUILD_SLEIGH_BACKENDS)
  add_subdirectory(Sleigh)
  list(APPEND REMILL_ARCH_LINK_LIBS remill_arch_sleigh)
endif()

if(REMILL_ENABLE_ARCH_X86)
  add_subdirectory(X86)
  list(APPEND REMILL_ARCH_LINK_LIBS remill_arch_x86)
endif()

if(REMILL_ARCH_LINK_LIBS)
  target_link_libraries(remill_arch PUBLIC ${REMILL_ARCH_LINK_LIBS})
endif()

if(REMILL_ENABLE_INSTALL_TARGET)
  install(TARGETS remill_arch
    EXPORT remillTargets)
endif()

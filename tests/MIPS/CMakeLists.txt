# Copyright (c) 2026-present Trail of Bits, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

find_package(GTest CONFIG REQUIRED)
list(APPEND PROJECT_LIBRARIES GTest::gtest)

enable_testing()

add_executable(
  run-mips32el-tests
  Main.cpp
  TestLifting32.cpp
)

add_test(NAME "mips32el-tests" COMMAND "run-mips32el-tests")
target_link_libraries(
  run-mips32el-tests
  PRIVATE
  GTest::gtest
  remill
  test-runner
  glog::glog
)
set_property(TARGET run-mips32el-tests PROPERTY ENABLE_EXPORTS ON)

add_executable(
  run-mips64el-tests
  Main.cpp
  TestLifting64.cpp
)

add_test(NAME "mips64el-tests" COMMAND "run-mips64el-tests")
target_link_libraries(
  run-mips64el-tests
  PRIVATE
  GTest::gtest
  remill
  test-runner
  glog::glog
)
set_property(TARGET run-mips64el-tests PROPERTY ENABLE_EXPORTS ON)


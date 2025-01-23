#########################################################################
# Copyright 2024 Comcast Cable Communications Management, LLC
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
#
# SPDX-License-Identifier: Apache-2.0
#########################################################################

cmake_minimum_required(VERSION 3.22)
project(rdk_logger_gtest)
# Find the gtest package
find_package(GTest REQUIRED)

include_directories(${RDKLOGGER_SOURCE_PATH}/)
include_directories(${RDKLOGGER_SOURCE_PATH}/include)
include_directories(${RDKLOGGER_SOURCE_PATH}/src/include)
include_directories(${RDKLOGGER_SOURCE_PATH}/cfg)
include_directories(${RDKLOGGER_UNITTEST_DIR}/)

# Set the path to the gtest installation directory
set(GTEST_ROOT "${RDKLOGGER_UNITTEST_DIR}/googletest/")

# Add the test executable
#add_executable(unitTestRunner testRunner.cpp)

# Link the gtest libraries to the test executable
#target_link_libraries(unitTestRunner GTest::GTest GTest::Main)


enable_testing()
link_directories("${RDKLOGGER_SOURCE_PATH}/src/.libs/")
link_directories("${DEP_LIB_PATH}/log4c-1.2.3/install/lib/")
link_directories("${RDKLOGGER_UNITTEST_DIR}/googletest/build/lib/")

# Add the test executable
add_executable(
  rdk_logger_gtest
  rdkloggerInit.cpp
  rdkloggerMsgPrintf.cpp
  rdkloggerMsgVsprintf.cpp
  rdkIsDbgEnabled.cpp
  main.cpp
)
target_compile_definitions(rdk_logger_gtest PRIVATE GTEST_DEBUG_INI_FILE="${RDKLOGGER_UNITTEST_DIR}/sample_gtest_debug.ini")


install (CODE "execute_process (COMMAND ${CMAKE_COMMAND} -E copy ${RDKLOGGER_UNITTEST_DIR}/Gtest_log4crc ${RDKLOGGER_UNITTEST_DIR}/build/log4crc)")

if (NOT GTEST_INCLUDE_DIR)
    target_link_libraries(rdk_logger_gtest  -L${RDKLOGGER_SOURCE_PATH}/src/.libs -lrdkloggers -L${DEP_LIB_PATH}/log4c-1.2.3/install/lib/ -llog4c -l:libgtest.a gcov)
else ()
    target_link_libraries(rdk_logger_gtest -L${RDKLOGGER_SOURCE_PATH}/src/.lib -L${DEP_LIB_PATH}/log4c-1.2.3/install/lib/ rdkloggers log4c gtest gcov)
endif()

# Link the gtest libraries to the test executable
target_link_libraries(
  rdk_logger_gtest
  GTest::gtest_main
)

message("RDKLOGGER_SOURCE_PATH ${RDKLOGGER_SOURCE_PATH} RDK_PROJECT_ROOT_PATH ${RDK_PROJECT_ROOT_PATH} RDKLOGGER_UNITTEST_DIR ${RDKLOGGER_UNITTEST_DIR}.")
include_directories(${RDKLOGGER_UNITTEST_DIR}/googletest/googletest/include)
include_directories(${RDKLOGGER_UNITTEST_DIR}/googletest/googletest/src)
include_directories(${RDKLOGGER_SOURCE_PATH}/include)
include_directories(${DEP_LIB_PATH}/log4c-1.2.3/src/log4c)
include(GoogleTest)
gtest_discover_tests(rdk_logger_gtest)
find_package(Threads)

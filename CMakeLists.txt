# Copyright (c) 2016 Samsung Electronics Co., Ltd.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#
# @file
# @author Pawel Kubik (p.kubik@samsung.com)
#

# Set up main project
#
cmake_minimum_required(VERSION 2.6)

project(fused-location)

if(NOT DEFINED VERSION)
    SET(VERSION "0.0.2")
endif(NOT DEFINED VERSION)

# Path for additional modules
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)
INCLUDE(FindPkgConfig)

message(STATUS "Configuring ${PROJECT_NAME}...")

# Options
#
option(BUILD_TESTS "Build tests." ON)

# Targets
#
SET(TARGET_COMMON     "fused-location-common")
SET(TARGET_SERVER     "fused-location-static")
SET(TARGET_SERVER_EXE "fused-location")
SET(TARGET_CORE       "fused-location-core")
SET(TARGET_IPC        "fused-location-ipc")
SET(TARGET_CLIENT     "fused-location-client")
SET(TARGET_TESTS      "fused-location-tests")

# Subdirectories
#
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)

# Set up general include directories
#
include_directories(SYSTEM ${SRC_DIR})



# Specify build paths
#
if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR "lib")
endif(NOT DEFINED CMAKE_INSTALL_LIBDIR)

if(NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
    set(CMAKE_INSTALL_INCLUDEDIR "include")
endif(NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

if(NOT DEFINED LIB_INSTALL_DIR)
    set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
endif(NOT DEFINED LIB_INSTALL_DIR)

if(NOT DEFINED INCLUDE_INSTALL_DIR)
    set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}")
endif(NOT DEFINED INCLUDE_INSTALL_DIR)

# CXX Compiler settings
#
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "DEBUG")
endif(NOT CMAKE_BUILD_TYPE)

if(DBUS_SESSION_BUS)
    add_definitions(-DDBUS_SESSION_BUS)
endif(DBUS_SESSION_BUS)

if (CMAKE_BUILD_TYPE STREQUAL DEBUG)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wall -Wextra -pthread -pedantic -O0 -ggdb")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -Wall -Wextra -pthread -pedantic -O0 -ggdb")
else ()
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wall -Wextra -pthread -pedantic -O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -Wall -Wextra -pthread -pedantic -O2 -DNDEBUG")
endif (CMAKE_BUILD_TYPE STREQUAL DEBUG)

# Recurse
#
add_subdirectory(${SRC_DIR}/common)
add_subdirectory(${SRC_DIR}/core)
add_subdirectory(${SRC_DIR}/ipc)
add_subdirectory(${SRC_DIR}/server)
add_subdirectory(${SRC_DIR}/server-exe)
add_subdirectory(${SRC_DIR}/client)

if (BUILD_TESTS)
    add_subdirectory(tests/ut)
endif()

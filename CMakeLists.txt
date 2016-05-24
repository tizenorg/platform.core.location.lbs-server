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
	SET(VERSION "0.0.3")
endif(NOT DEFINED VERSION)

# Path for additional modules
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)
include(FindPkgConfig)

message(STATUS "Configuring ${PROJECT_NAME}...")

# Options
#
option(BUILD_TESTS "Build tests." ON)

# Targets
#
set(TARGET_COMMON     "fused-location-common")
set(TARGET_SERVER     "fused-location-static")
set(TARGET_SERVER_EXE "fused-location")
set(TARGET_CORE       "fused-location-core")
set(TARGET_IPC        "fused-location-ipc")
set(TARGET_CLIENT     "fused-location-client")
set(TARGET_TESTS      "fused-location-tests")

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
    message(WARNING "Undefined CMAKE_BUILD_TYPE")
else()
	message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif(NOT CMAKE_BUILD_TYPE)

if(DBUS_SESSION_BUS)
	add_definitions(-DDBUS_SESSION_BUS)
endif(DBUS_SESSION_BUS)

if (CMAKE_BUILD_TYPE STREQUAL DEBUG)
	set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS_DDEBUG} -Wp,-U_FORTIFY_SOURCE -O0 -ggdb -Wall")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DDEBUG} -Wp,-U_FORTIFY_SOURCE -O0 -ggdb -Wall")
endif ()

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wextra -pthread -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -Wextra -pthread -pedantic")

# Recurse
#
add_subdirectory(${SRC_DIR}/common)
add_subdirectory(${SRC_DIR}/core)
add_subdirectory(${SRC_DIR}/ipc)
add_subdirectory(${SRC_DIR}/server)
add_subdirectory(${SRC_DIR}/server-exe)
add_subdirectory(${SRC_DIR}/client)

if (BUILD_TESTS)
	add_subdirectory(tests/manual/accelerometer)
	add_subdirectory(tests/ut)
endif()

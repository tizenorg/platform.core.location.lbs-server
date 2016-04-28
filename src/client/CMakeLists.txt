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

set(CURRENT_TARGET ${TARGET_CLIENT})

message(STATUS "")
message(STATUS "Configuring ${CURRENT_TARGET}...")

# Specify source files
file(GLOB_RECURSE SRCS *.cpp)

# Set up targets
# TODO: This should be a library, see src/client/main.cpp
add_executable(${CURRENT_TARGET} ${SRCS})

# Set up dependencies
add_dependencies(${CURRENT_TARGET} ${TARGET_COMMON} ${TARGET_IPC})

target_link_libraries(${CURRENT_TARGET} ${TARGET_COMMON} ${TARGET_IPC})

# Set up install
install(TARGETS ${CURRENT_TARGET} RUNTIME DESTINATION bin)

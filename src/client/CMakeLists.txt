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
file(GLOB_RECURSE SRCS    *.cpp)

set(_LIB_VERSION_   "${VERSION}")
set(_LIB_SOVERSION_ "0")
set(PC_FILE "${CURRENT_TARGET}.pc")

# Set up targets
# TODO: This should be a library, see src/client/main.cpp
add_library(${CURRENT_TARGET} SHARED ${SRCS})
set_target_properties(${CURRENT_TARGET} PROPERTIES
    SOVERSION   ${_LIB_SOVERSION_}
    VERSION     ${_LIB_VERSION_}
)
add_definitions(-fvisibility=hidden)

# Set up dependencies
add_dependencies(${CURRENT_TARGET} ${TARGET_COMMON} ${TARGET_IPC})

target_link_libraries(${CURRENT_TARGET} ${TARGET_COMMON} ${TARGET_IPC})

# Set up install
install(TARGETS ${CURRENT_TARGET}
        LIBRARY DESTINATION ${LIB_INSTALL_DIR}
        COMPONENT RuntimeLibraries)

install(DIRECTORY . DESTINATION ${INCLUDE_INSTALL_DIR}/${PROJECT_NAME}
        FILES_MATCHING PATTERN "*.h"
                       PATTERN "CMakeFiles" EXCLUDE)

configure_file(${PC_FILE}.in ${CMAKE_BINARY_DIR}/${PC_FILE} @ONLY)
install(FILES       ${CMAKE_BINARY_DIR}/${PC_FILE}
        DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)

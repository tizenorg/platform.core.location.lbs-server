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

set(CURRENT_TARGET ${TARGET_SERVER})

message(STATUS "")
message(STATUS "Configuring ${CURRENT_TARGET}...")

# Specify source files
file(GLOB_RECURSE SRCS *.cpp)

set(_LIB_VERSION_   "${VERSION}")
set(_LIB_SOVERSION_ "0")

# Set up targets
add_library(${CURRENT_TARGET} ${SRCS})
set_target_properties(${CURRENT_TARGET} PROPERTIES
	SOVERSION   ${_LIB_SOVERSION_}
	VERSION     ${_LIB_VERSION_}
)

# Set up dependencies
pkg_check_modules(TIZEN_LIBS REQUIRED lbs-dbus capi-system-sensor)

target_include_directories(${CURRENT_TARGET} SYSTEM PUBLIC ${TIZEN_LIBS_INCLUDE_DIRS})

add_dependencies(${CURRENT_TARGET} ${TARGET_COMMON} ${TARGET_CORE} ${TARGET_IPC})

target_link_libraries(${CURRENT_TARGET} ${TARGET_COMMON} ${TARGET_CORE} ${TARGET_IPC}
                      ${TIZEN_LIBS_LIBRARIES})

# Set up install
install(TARGETS     ${CURRENT_TARGET}
        DESTINATION ${LIB_INSTALL_DIR}
        COMPONENT   RuntimeLibraries)

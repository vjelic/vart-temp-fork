#
# Copyright (C) 2022 Xilinx, Inc.
# Copyright (C) 2023 – 2024 Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
get_filename_component(COMPONENT_NAME "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${PROJECT_SOURCE_DIR}/cmake")

add_library(
  ${COMPONENT_NAME}
  include/vart/trace/common.hpp
  include/vart/trace/event.hpp
  include/vart/trace/fmt.hpp
  include/vart/trace/payload.hpp
  include/vart/trace/ringbuf.hpp
  include/vart/trace/trace.hpp
  include/vart/trace/traceclass.hpp
  include/vart/trace/vaitrace_dbg.hpp
  src/event.cpp
  src/internal.hpp
  src/pid.h
  src/str.cpp
  src/str.hpp
  src/subgraph.cpp
  src/time.cpp
  src/time.hpp
  src/traceclass.cpp
  src/trace.cpp
  src/trace_py.cpp
  src/util.cpp
  src/util.hpp)

add_library(${PROJECT_NAME}::${COMPONENT_NAME} ALIAS ${COMPONENT_NAME})

set_target_properties(
  ${COMPONENT_NAME}
  PROPERTIES VERSION "${PROJECT_VERSION}"
             SOVERSION "${PROJECT_VERSION_MAJOR}"
             OUTPUT_NAME ${PROJECT_NAME}-${COMPONENT_NAME}
             CXX_STANDARD 17)

target_link_libraries(${COMPONENT_NAME} PRIVATE ${CMAKE_THREAD_LIBS_INIT}
                                                xir::xir util)
if(XRT_FOUND AND ENABLE_DPU_RUNNER)
  target_link_libraries(${COMPONENT_NAME} PRIVATE ${CMAKE_THREAD_LIBS_INIT}
                                                  XRT::xrt_coreutil)
  if(!WIN32)
    target_compile_definitions(${COMPONENT_NAME} PRIVATE ENABLE_XRT_TIMESTAMP)
  endif()
endif()

target_include_directories(
  ${COMPONENT_NAME}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(CMAKE_SOURCE_DIR STREQUAL vart_SOURCE_DIR)
install(
  TARGETS ${COMPONENT_NAME}
  EXPORT ${COMPONENT_NAME}-targets
  COMPONENT trace
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)
install(
  EXPORT ${COMPONENT_NAME}-targets
  NAMESPACE ${PROJECT_NAME}::
  COMPONENT trace
  DESTINATION share/cmake/${PROJECT_NAME})

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/vart/trace/
  DESTINATION include/vart/trace
  COMPONENT trace
  FILES_MATCHING
  PATTERN "*.hpp"
  PATTERN "*.h")
endif()

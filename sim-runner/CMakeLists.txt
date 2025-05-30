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
get_filename_component(COMPONENT_NAME "${CMAKE_CURRENT_SOURCE_DIR}" NAME)

add_definitions("-DRESIZE_DATA_TYPE=uint8_t")
add_definitions("-DSMFC_DATA_TYPE=float")
add_definitions("-DDPU_DATA_TYPE=char")

aux_source_directory(src SRC_FILES)
aux_source_directory(src/util SRC_FILES)
aux_source_directory(src/conf SRC_FILES)
aux_source_directory(src/buffer SRC_FILES)
aux_source_directory(src/inst SRC_FILES)
aux_source_directory(src/inst/pub SRC_FILES)
aux_source_directory(src/inst/xvdpu SRC_FILES)
aux_source_directory(src/inst/xv2dpu SRC_FILES)
aux_source_directory(src/inst/xv2dpu/load_ctrlpkt/src SRC_FILES)
aux_source_directory(src/inst/xv2dpu/alu_ctrlpkt/AIECfg SRC_FILES)
aux_source_directory(src/inst/xv2dpu/alu_ctrlpkt/Util SRC_FILES)

add_library(${COMPONENT_NAME} ${SRC_FILES})
set_target_properties(
  ${COMPONENT_NAME}
  PROPERTIES VERSION "${PROJECT_VERSION}"
             SOVERSION "${PROJECT_VERSION_MAJOR}"
             OUTPUT_NAME ${PROJECT_NAME}-${COMPONENT_NAME})
target_include_directories(
  ${COMPONENT_NAME}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
          ${CMAKE_CURRENT_SOURCE_DIR}/src/conf
          ${CMAKE_CURRENT_SOURCE_DIR}/src/inst
          ${CMAKE_CURRENT_SOURCE_DIR}/src/inst/pub
          ${CMAKE_CURRENT_SOURCE_DIR}/src/buffer
          ${CMAKE_CURRENT_SOURCE_DIR}/src/util
          ${CMAKE_CURRENT_SOURCE_DIR}/src/inst/xv2dpu
          ${CMAKE_CURRENT_SOURCE_DIR}/src/inst/xv2dpu/alu_ctrlpkt/AIECfg
          ${CMAKE_CURRENT_SOURCE_DIR}/src/inst/xv2dpu/alu_ctrlpkt/Ops
          ${CMAKE_CURRENT_SOURCE_DIR}/src/inst/xv2dpu/alu_ctrlpkt/Util
          ${CMAKE_CURRENT_SOURCE_DIR}/src/inst/xv2dpu/load_ctrlpkt/inc
          ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(
  ${COMPONENT_NAME} PUBLIC util mem-manager runner xir::xir unilog::unilog
                           target-factory::target-factory)

if(CMAKE_SOURCE_DIR STREQUAL vart_SOURCE_DIR)
install(
  TARGETS ${COMPONENT_NAME}
  EXPORT ${COMPONENT_NAME}-targets
  COMPONENT sim
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib)
install(
  FILES include/sim_runner.hpp include/sim_tensor_buffer.hpp include/SimCfg.hpp
        src/conf/ArchCfg.hpp
  COMPONENT sim
  DESTINATION include/vart/sim)

install(
  EXPORT ${COMPONENT_NAME}-targets
  NAMESPACE ${PROJECT_NAME}::
  COMPONENT sim
  DESTINATION share/cmake/${PROJECT_NAME})
endif()

if(BUILD_TEST)
  add_subdirectory(test)
endif()

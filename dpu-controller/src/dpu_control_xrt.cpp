/*
 * Copyright (C) 2022 Xilinx, Inc.
 * Copyright (C) 2023 – 2024 Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <experimental/xrt_system.h>
#include <glog/logging.h>

#include <UniLog/UniLog.hpp>
#include <vitis/ai/env_config.hpp>

#include "./dpu_control_xrt_cloud.hpp"
#include "./dpu_control_xrt_edge.hpp"
#include "./dpu_control_xrt_xv2_dpu.hpp"
#include "./dpu_control_xrt_xv_dpu.hpp"
#include "./xrt_cu.hpp"

DEF_ENV_PARAM_2(XLNX_DPU_TYPE, "", std::string);
DEF_ENV_PARAM(DEBUG_DPU_CONTROLLER, "0");
DEF_ENV_PARAM(DISABLE_DPU_CONTROLLER_XRT, "0");
namespace {
static std::string get_cu_kernel_name(const xir::XrtCu* xcu,
                                      size_t num_of_cus) {
  auto type = ENV_PARAM(XLNX_DPU_TYPE);
  if (!type.empty()) {
    LOG(WARNING) << "use env XLNX_DPU_TYPE " << type
                 << " to overwrite dpu type."
                 << " Be careful!";
    return type;
  }
  // by default, all the cus will have the same kernel name
  auto ret = xcu->get_kernel_name(0);
  for (auto i = 1u; i < num_of_cus && false
       /**
        * disable checking for no reason. It would be better if we can unify
        * the kernel name.
        */
       ;
       ++i) {
    UNI_LOG_CHECK(xcu->get_kernel_name(i) == ret, VART_XRT_READ_CU_ERROR)
        << "all cu must have same kernel name. i = " << i;
  }
  LOG_IF(INFO, ENV_PARAM(DEBUG_DPU_CONTROLLER))
      << "xrt dpu cu is detected, kernel = " << ret;
  return ret;
}

static struct Registar {
  Registar() {
    auto n = xrt::system::enumerate_devices();
    if (n == 0) {
      LOG_IF(INFO, ENV_PARAM(DEBUG_DPU_CONTROLLER))
          << "no xrt device found, no dpu controller will be registered.";
      return;
    }
    auto disabled = ENV_PARAM(DISABLE_DPU_CONTROLLER_XRT);
    if (!disabled) {
      xir::DpuController::registar(
          "02_xrt", []() -> std::shared_ptr<xir::DpuController> {
            auto xrt_cu = std::make_unique<xir::XrtCu>(std::string{"DPU"});
            auto num_of_cus = xrt_cu->get_num_of_cu();
            //  For compatibility with old and new dpu name, Support
            // both "dpu" & "DPU"
            if (num_of_cus == 0) {
              xrt_cu = std::make_unique<xir::XrtCu>(std::string{"dpu"});
              num_of_cus = xrt_cu->get_num_of_cu();
            }
            UNI_LOG_CHECK(num_of_cus > 0, VART_XRT_READ_CU_ERROR)
                << "no dpu cu detected. please check /usr/lib/dpu.xclbin";
            auto cu_kernel_name = get_cu_kernel_name(xrt_cu.get(), num_of_cus);
            if (cu_kernel_name.find("DPUCZDX8G") == 0 ||
                cu_kernel_name == "dpu_xrt_top") {  // For compatibility with
                                                    // old and new dpu name.
              LOG_IF(INFO, ENV_PARAM(DEBUG_DPU_CONTROLLER))
                  << "create DpuControllerXrtEdge for " << cu_kernel_name;
              return std::make_shared<DpuControllerXrtEdge>(std::move(xrt_cu));
            } else if (cu_kernel_name.find("DPUCVDX8H") == 0) {  // V4E
              LOG_IF(INFO, ENV_PARAM(DEBUG_DPU_CONTROLLER))
                  << "create DpuControllerXrtCloud for " << cu_kernel_name;
              return std::make_shared<DpuControllerXrtCloud>(
                  std::move(xrt_cu), DPU_CLOUD_TYPE::V4E);
            } else if (cu_kernel_name.find("DPUCAHX8H") == 0) {
              LOG_IF(INFO, ENV_PARAM(DEBUG_DPU_CONTROLLER))
                  << "DpuControllerXrtCloud DpuControllerXrtCloud for "
                  << cu_kernel_name;
              return std::make_shared<DpuControllerXrtCloud>(
                  std::move(xrt_cu), DPU_CLOUD_TYPE::V3E);
            } else if (cu_kernel_name.find("DPUCVDX8G") == 0) {
              LOG_IF(INFO, ENV_PARAM(DEBUG_DPU_CONTROLLER))
                  << "DpuControllerXrtCloud DpuControllerXrtXvDpu for "
                  << cu_kernel_name;
              return std::make_shared<DpuControllerXrtXvDpu>(std::move(xrt_cu));
            } else if (cu_kernel_name.find("DPUCV2DX8G") == 0) {
              LOG_IF(INFO, ENV_PARAM(DEBUG_DPU_CONTROLLER))
                  << "DpuControllerXrtCloud DpuControllerXrtXv2Dpu for "
                  << cu_kernel_name;
              return std::make_shared<DpuControllerXrtXv2Dpu>(
                  std::move(xrt_cu));
            } else {
              LOG(FATAL) << "unknown dpu type! cu_kernel_name="
                         << cu_kernel_name
                         << ". valid types: dpu_xrt_top, dpu_3e and dpu_4e.";
            }
            return nullptr;
          });
      LOG_IF(INFO, ENV_PARAM(DEBUG_DPU_CONTROLLER))
          << "register the xrt edge dpu controller";
    } else {
      LOG_IF(INFO, ENV_PARAM(DEBUG_DPU_CONTROLLER))
          << "cancel register the xrt edge  controller, because "
             "DISABLE_DPU_CONTROLLER_FAKE=1";
    }
  }
}  // namespace
g_registar;
}  // namespace

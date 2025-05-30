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
#include <experimental/xrt-next.h>
#include <xrt.h>

#include <iostream>
#include <vitis/ai/env_config.hpp>

#include "vitis/ai/parse_value.hpp"
#include "vitis/ai/profiling.hpp"
#include "xir/dpu_controller.hpp"
#include "xir/xrt_device_handle.hpp"
DEF_ENV_PARAM_2(CU_NAME, "DPUCZDX8G", std::string);

using namespace std;
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

int main(int argc, char* argv[]) {
  auto h = xir::XrtDeviceHandle::get_instance();
  auto dpu = xir::DpuController::get_instance();

  auto w = 0ul;
  vitis::ai::parse_value(argv[1], w);
  auto p = 0ul;
  vitis::ai::parse_value(argv[2], p);
  auto c = 0ul;
  vitis::ai::parse_value(argv[3], c);
  auto idx = 0ul;
  vitis::ai::parse_value(argv[4], idx);
  auto count = std::stoi(argv[5]);

  auto cu_name = ENV_PARAM(CU_NAME);
  for (auto i = 0; i < count; ++i) {
    auto start = std::chrono::steady_clock::now();
    auto start_cycle = h->read_register(cu_name, idx, 0x1A0);
    dpu->run(idx, c, {p, w});
    auto end = std::chrono::steady_clock::now();
    auto end_cycle = h->read_register(cu_name, idx, 0x1A0);
    auto ms = std::chrono::duration_cast<std::chrono::microseconds>(end - start)
                  .count();
    auto cycle = end_cycle - start_cycle;
    cout << ms << " " << start_cycle << " " << end_cycle << " " << cycle
         << "\n";
  }
  return 0;
}

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

#pragma once

#include "cpu_op_base.hpp"

namespace vart {
namespace cpu {

template <typename DType>
class ChannelShuffle : public CPUOPBase {
 public:
  ChannelShuffle(const xir::Subgraph* subg, const xir::Op* op, IMapTBs_t inputs,
               CPUTBPtr_t output);
  ~ChannelShuffle() = default;

  virtual void run() override final;

  virtual void print_param() override final;
  virtual void check_param() override final;

  virtual void read() override final;

  virtual uint64_t get_workload() override final;

 private:
  void shuffle();

 private:
  std::vector<int> input_shape_;
  std::vector<int> output_shape_;
  int group_;

  DType* data_in_{nullptr};
  DType* data_out_{nullptr};
};

}  // namespace cpu
}  // namespace vart

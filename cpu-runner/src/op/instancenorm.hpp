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

template <typename DType> class InstanceNorm : public CPUOPBase {
public:
  InstanceNorm(const xir::Subgraph *subg, const xir::Op *op, IMapTBs_t inputs,
               CPUTBPtr_t output);
  ~InstanceNorm() = default;

  virtual void run() override final;

  virtual void read() override final;
  void inst_norm(int32_t i);

protected:
  float epsilon_;
  std::vector<std::int32_t> input_shape;
  DType *input_input_ptr_;
  DType *input_weights_ptr_;
  DType *input_bias_ptr_;
  DType *output_output_ptr_;
};

} // namespace cpu
} // namespace vart
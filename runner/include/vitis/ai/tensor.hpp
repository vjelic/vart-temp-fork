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

#include <string>
#include <vector>

namespace vitis {
namespace ai {

class Tensor {
 public:
  enum class DataType {
    INT8,
    UINT8,
    INT16,
    UINT16,
    INT32,
    UINT32,
    INT64,
    UINT64,
    FLOAT,
    DOUBLE,
    UNKNOWN
  };

 public:
  Tensor(std::string name, std::vector<std::int32_t> dims, DataType data_type);
  Tensor() = delete;

 public:
  const std::string& get_name() const;

  std::int32_t get_dim_num() const;

  std::int32_t get_element_num() const;

  std::int32_t get_dim_size(std::int32_t idx) const;

  const std::vector<std::int32_t>& get_dims() const;

  DataType get_data_type() const;

  std::string to_string() const;

 private:
  const std::string name_;
  const std::vector<std::int32_t> dims_;
  const DataType data_type_;
};

std::size_t size_of(Tensor::DataType data_type);
}  // namespace ai
}  // namespace vitis

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
#include <assert.h>
#include <ert.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <xrt/xrt_device.h>
#include <xrt/xrt_kernel.h>

#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "xir/xrt_device_handle.hpp"
using namespace std;
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#define BIT(nr) (1 << (nr))

int DEBUG_COUT = 0;
#define ENABLE_LOCK 1

#define XDPU_GLOBAL_INT_ENABLE BIT(0)
#define XDPU_CONTROL_AP 0x00
#define XDPU_CONTROL_AP_START 0x00
#define XDPU_CONTROL_GIE 0x04
#define XDPU_CONTROL_IER 0x08
#define XDPU_CONTROL_ISR 0x0C
#define BASE_ADDR 0x00000       // NOTE: USE ONLY FOR BEFORE 2020.1
#define XDPU_CONTROL_START 0x10 /* write 1 to enable DPU start */
#define XDPU_CONTROL_RESET 0x1C /* reset DPU active low */
#define XDPU_CONTROL_HP 0x20
#define XDPU_CONTROL_INSTR_L 0x140
#define XDPU_CONTROL_INSTR_H 0x144
#define XDPU_CONTROL_ADDR_0_L 0x100
#define XDPU_CONTROL_ADDR_0_H 0x104
#define XDPU_CONTROL_ADDR_1_L 0x108
#define XDPU_CONTROL_ADDR_1_H 0x10C
#define XDPU_CONTROL_ADDR_2_L 0x110
#define XDPU_CONTROL_ADDR_2_H 0x114
#define XDPU_CONTROL_ADDR_3_L 0x118
#define XDPU_CONTROL_ADDR_3_H 0x11C
#define XDPU_CONTROL_ADDR_4_L 0x120
#define XDPU_CONTROL_ADDR_4_H 0x124
#define XDPU_CONTROL_ADDR_5_L 0x128
#define XDPU_CONTROL_ADDR_5_H 0x12C
#define XDPU_CONTROL_ADDR_6_L 0x130
#define XDPU_CONTROL_ADDR_6_H 0x134
#define XDPU_CONTROL_ADDR_7_L 0x138
#define XDPU_CONTROL_ADDR_7_H 0x13C

struct timestamps {
  uint64_t total;
  uint64_t to_driver;
  uint64_t to_cu;
  uint64_t cu_complete;
  uint64_t done;
};
static inline uint64_t tp2ns(struct timespec* tp) {
  return (uint64_t)tp->tv_sec * 1000000000UL + tp->tv_nsec;
}
static void print_one_timestamp(const timestamps& ts) {
  if (DEBUG_COUT)
    std::cout << ts.total / 1000 << "," << ts.to_driver / 1000 << ","
              << ts.to_cu / 1000 << "," << ts.cu_complete / 1000 << ","
              << ts.done / 1000 << std::endl;
}
static void print_timestamp(const uint64_t start, const uint64_t end,
                            cu_cmd_state_timestamps* c) {
  struct timestamps ts;
  ts.total = end - start;
  ts.to_driver = c->skc_timestamps[ERT_CMD_STATE_NEW] - start;
  ts.to_cu = c->skc_timestamps[ERT_CMD_STATE_RUNNING] -
             c->skc_timestamps[ERT_CMD_STATE_NEW];
  ts.cu_complete = c->skc_timestamps[ERT_CMD_STATE_COMPLETED] -
                   c->skc_timestamps[ERT_CMD_STATE_RUNNING];
  ts.done = end - c->skc_timestamps[ERT_CMD_STATE_COMPLETED];
  print_one_timestamp(ts);
}

bool g_is_stop = false;
void signal_handler(int) { g_is_stop = true; }
int g_idx = 1;
std::mutex g_mtx;
std::mutex g_mtx_ecmd;

void run(xrt::run* r) {
  auto ecmd = reinterpret_cast<ert_start_kernel_cmd*>(r->get_ert_packet());
  while (1) {
    if (g_is_stop) return;
    if (g_idx > 20000) return;

    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    uint64_t start = tp2ns(&tp);

    // ecmd->cu_mask = cu_mask;
    ecmd->stat_enabled = 1;

    ecmd->state = ERT_CMD_STATE_NEW;
    ecmd->opcode = ERT_START_CU;
    ecmd->count = 120;
    ecmd->type = ERT_CTRL;
    ecmd->data[XDPU_CONTROL_AP] = XDPU_CONTROL_AP_START;        // [0] APCTL=0,
    ecmd->data[XDPU_CONTROL_GIE / 4] = XDPU_GLOBAL_INT_ENABLE;  // [1] GIE =1
    ecmd->data[XDPU_CONTROL_IER / 4] = 1;                       // [2] IER = 1
    ecmd->data[XDPU_CONTROL_START / 4] = 0x00;
    ecmd->data[XDPU_CONTROL_RESET / 4] = 0x1;
    ecmd->data[XDPU_CONTROL_HP / 4] = 0x204040;
    ecmd->data[XDPU_CONTROL_INSTR_L / 4] = 0x80000000;
    ecmd->data[XDPU_CONTROL_INSTR_H / 4] = 0xc2;

    ecmd->data[XDPU_CONTROL_ADDR_0_L / 4] = 0x0;
    ecmd->data[XDPU_CONTROL_ADDR_0_H / 4] = 0xc2;
    ecmd->data[XDPU_CONTROL_ADDR_1_L / 4] = 0x0;
    ecmd->data[XDPU_CONTROL_ADDR_1_H / 4] = 0xc0;
    auto state = 0;
    state = ecmd->state;
    if (DEBUG_COUT)
      cout << "sizeof(ecmd) " << sizeof(*ecmd) << " "   //
           << "ecmd->state " << ecmd->state << " "      //
           << "ecmd->cu_mask " << ecmd->cu_mask << " "  //
           << "ecmd->count " << ecmd->count << " "      //
           << "ecmd->opcode " << ecmd->opcode << " "    //
           << "ecmd->type " << ecmd->type << " "        //
           << endl;

    if (ENABLE_LOCK) g_mtx_ecmd.lock();
    r->start();
    int wait_count = 0;
    while (state < 4 && wait_count < 5) {
      if (DEBUG_COUT) cout << "Waiting for DONE\n";
      state = r->wait(1000);
      wait_count++;
    }
    if (ENABLE_LOCK) g_mtx_ecmd.unlock();
    clock_gettime(CLOCK_MONOTONIC, &tp);
    uint64_t end = tp2ns(&tp);

    if (state > 4 || wait_count == 5) r->abort();
    print_timestamp(start, end, ert_start_kernel_timestamps(ecmd));
    g_mtx.lock();
    g_idx++;
    g_mtx.unlock();
    if (DEBUG_COUT)
      cout << "wait_count " << wait_count << " "  //
           << "state " << state << " "            //
           << endl;
  }
}

string input_data = "./case_new_resnet50/input_aquant.bin";
string code = "./case_new_resnet50/code.bin";
string reg0 = "./case_new_resnet50/reg0.bin";
void upload_data(const xrt::device* device) {
  uint64_t addr = 0xc000001000;
  size_t size = 150528;
  auto buf = std::vector<char>(size);
  auto flags = XRT_BO_FLAGS_DEV_ONLY;
  ifstream infile(input_data, ios::in | ios::binary);
  assert(infile);
  infile.read(buf.data(), size);
  // not sure if it's correct to replace xclUnmgdPwrite() with xrt::bo()
  // if true, replace the other invokes

  // auto ok = xclUnmgdPwrite(handle, flags, &buf[0], size, addr);
  auto bo_in = xrt::bo(*device, (void*)addr, size, flags, 0);
  auto in_phy = bo_in.address();
  std::cout << "in_phy=0x" << std::hex << in_phy
            << ", addr=0x" << addr << std::dec << ", size = " << size;
  assert(in_phy == 0);
  bo_in.write(buf.data());

  addr = 0xc280000000;
  size = 38108;
  buf = std::vector<char>(size);
  infile = ifstream(code, ios::in | ios::binary);
  assert(infile);
  infile.read(buf.data(), size);
  /*i = 0;
  while (infile.read((char*)&buf[i], sizeof(char))) {
    ++i;
  }
  ok = xclUnmgdPwrite(handle, flags, &buf[0], size, addr);
  if (ok != 0) {
    std::cout << "sync for write faild! addr=" << addr << ",size = " << size;
  }*/
  auto bo_code = xrt::bo(*device, (void*)addr, size, flags, 0);
  auto code_phy = bo_code.address();
  std::cout << "code_phy=0x" << std::hex << code_phy
            << ", addr=0x" << addr << std::dec << ", size = " << size;
  assert(code_phy == 0);
  bo_code.write(buf.data());

  addr = 0xc200000000;
  size = 25775488;
  buf = std::vector<char>(size);
  infile = ifstream(reg0, ios::in | ios::binary);
  assert(infile);
  infile.read(buf.data(), size);
  /*i = 0;
  while (infile.read((char*)&buf[i], sizeof(char))) {
    ++i;
  }
  ok = xclUnmgdPwrite(handle, flags, &buf[0], size, addr);
  if (ok != 0) {
    std::cout << "sync for write faild! addr=" << addr << ",size = " << size;
  }
  assert(ok == 0);*/
  auto bo_reg = xrt::bo(*device, (void*)addr, size, flags, 0);
  auto reg_phy = bo_reg.address();
  std::cout << "reg_phy=0x" << std::hex << reg_phy
            << ", addr=0x" << addr << std::dec << ", size = " << size;
  assert(reg_phy == 0);
  bo_reg.write(buf.data());
}

int main(int argc, char* argv[]) {
  auto filename = std::string(argv[1]);
  int thread_nums = (argc < 3) ? 2 : stoi(argv[2]);
  DEBUG_COUT = (argc > 4) && (stoi(argv[3]) > 0) ? 0 : 1;
  auto h = xir::XrtDeviceHandle::get_instance();
  std::string cu_name("DPU");
  auto device =
      reinterpret_cast<const xrt::device*>(h->get_device_handle(cu_name, 0));
  auto kernel =
      reinterpret_cast<const xrt::kernel*>(h->get_kernel_handle(cu_name, 0));
  auto r = xrt::run(*kernel);

  upload_data(device);
  if (DEBUG_COUT) cout << "thread nums:" << thread_nums << endl;
  signal(SIGINT, signal_handler);
  vector<thread> threads;
  for (int i = 0; i < thread_nums; ++i) {
    threads.emplace_back(thread(run, &r));
  }
  auto exe_start = std::chrono::system_clock::now();
  for (int i = 0; i < thread_nums; ++i) {
    threads[i].join();
  }
  auto act_time = std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::system_clock::now() - exe_start)
                      .count();
  float sec = (float)act_time / 1000000.0;
  float fps = ((float)g_idx) / sec;
  cout << "FPS: " << fps << endl;
  return 0;
}

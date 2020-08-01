/**
 * Copyright 2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "src/runtime/kernel/arm/fp32/split.h"
#include "src/runtime/kernel/arm/base/split_base.h"
#include "src/runtime/kernel/arm/opclib/split.h"
#include "src/runtime/kernel/arm/opclib/split_parameter.h"
#include "src/kernel_registry.h"
#include "include/errorcode.h"
#include "src/runtime/runtime_api.h"

using mindspore::kernel::KERNEL_ARCH::kCPU;
using mindspore::lite::KernelRegistrar;
using mindspore::lite::RET_ERROR;
using mindspore::lite::RET_OK;
using mindspore::schema::PrimitiveType_Split;

namespace mindspore::kernel {

int SplitCPUKernel::Init() {
  SplitBaseCPUKernel::Init();
  auto in_tensor = inputs_.front();
  input_ptr_ = reinterpret_cast<float *>(in_tensor->Data());
  for (int i = 0; i < param->num_split_; i++) {
    output_ptr_.push_back(reinterpret_cast<float *>(outputs_.at(i)->Data()));
  }
  return RET_OK;
}

int SplitCPUKernel::ReSize() { return RET_OK; }

int SplitCPUKernel::Split(int task_id) {
  int num_unit_thread = MSMIN(thread_n_stride_, num_unit_ - task_id * thread_n_stride_);
  if (num_unit_thread <= 0) {
    return RET_OK;
  }
  int thread_offset = task_id * thread_n_stride_;
  auto ret = DoSplit(input_ptr_, output_ptr_.data(), inputs_.front()->shape().data(), thread_offset, num_unit_thread,
                     param);
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "Split error task_id[" << task_id << "] error_code[" << ret << "]";
    return RET_ERROR;
  }
  return RET_OK;
}

int SplitRun(int task_id, LiteParallelGroupEnv *penv, void *cdata) {
  auto g_kernel = reinterpret_cast<SplitCPUKernel *>(cdata);
  auto ret = g_kernel->Split(task_id);
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "SplitRun error task_id[" << task_id << "] error_code[" << ret << "]";
    return RET_ERROR;
  }
  return RET_OK;
}

int SplitCPUKernel::Run() {
  int ret = LiteBackendParallelLaunch(SplitRun, this, thread_n_num_);
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "Scale error error_code[" << ret << "]";
    return RET_ERROR;
  }

  return RET_OK;
}
}  // namespace mindspore::kernel

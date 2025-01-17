/* Copyright 2023 NVIDIA Corporation
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
 *
 */

#include "legate_library.h"
#include "legate_raft_cffi.h"

#include "legate/utilities/dispatch.h"

namespace legate_raft {

namespace {

struct mul_fn {
  template <legate::Type::Code CODE, int32_t DIM>
  void operator()(legate::PhysicalStore lhs, legate::PhysicalStore rhs1, legate::PhysicalStore rhs2)
  {
    using VAL = legate::type_of_t<CODE>;

    auto shape = lhs.shape<DIM>();

    if (shape.empty()) return;

    auto rhs1_acc = rhs1.read_accessor<VAL, DIM>();
    auto rhs2_acc = rhs2.read_accessor<VAL, DIM>();
    auto lhs_acc  = lhs.write_accessor<VAL, DIM>();

    for (legate::PointInRectIterator<DIM> it(shape, false /*fortran_order*/); it.valid(); ++it) {
      auto p     = *it;
      lhs_acc[p] = rhs1_acc[p] * rhs2_acc[p];
    }
  }
};

}  // namespace

class MultiplyTask : public Task<MultiplyTask, MUL> {
 public:
  static void cpu_variant(legate::TaskContext context)
  {
    auto rhs1 = context.input(0);
    auto rhs2 = context.input(1);
    auto lhs  = context.output(0);

    legate::double_dispatch(lhs.data().dim(), lhs.data().code(), mul_fn{}, lhs, rhs1, rhs2);
  }
};

}  // namespace legate_raft

namespace {

static void __attribute__((constructor)) register_tasks()
{
  legate_raft::MultiplyTask::register_variants();
}

}  // namespace

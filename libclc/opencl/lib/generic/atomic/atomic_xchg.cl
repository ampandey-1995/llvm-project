//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <clc/opencl/as_type.h>
#include <clc/opencl/atomic/atomic_xchg.h>

_CLC_OVERLOAD _CLC_DEF float atomic_xchg(volatile global float *p, float val) {
  return as_float(atomic_xchg((volatile global uint *)p, as_uint(val)));
}

_CLC_OVERLOAD _CLC_DEF float atomic_xchg(volatile local float *p, float val) {
  return as_float(atomic_xchg((volatile local uint *)p, as_uint(val)));
}

#define IMPL(TYPE, AS)                                                         \
  _CLC_OVERLOAD _CLC_DEF TYPE atomic_xchg(volatile AS TYPE *p, TYPE val) {     \
    return __sync_swap_4(p, val);                                              \
  }

IMPL(int, global)
IMPL(unsigned int, global)
IMPL(int, local)
IMPL(unsigned int, local)
#undef IMPL

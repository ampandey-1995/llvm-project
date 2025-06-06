// RUN: %clang_cc1 -x hip %s -emit-llvm -o - -triple=amdgcn-amd-amdhsa \
// RUN:   -fcuda-is-device -target-cpu gfx906 -fnative-half-type \
// RUN:   -fnative-half-arguments-and-returns | FileCheck -check-prefixes=FUN,CHECK,SAFEIR %s

// RUN: %clang_cc1 -x hip %s -emit-llvm -o - -triple=amdgcn-amd-amdhsa \
// RUN:   -fcuda-is-device -target-cpu gfx906 -fnative-half-type \
// RUN:   -fnative-half-arguments-and-returns -munsafe-fp-atomics | FileCheck -check-prefixes=FUN,CHECK,UNSAFEIR %s

// RUN: %clang_cc1 -x hip %s -O3 -S -o - -triple=amdgcn-amd-amdhsa \
// RUN:   -fcuda-is-device -target-cpu gfx1100 -fnative-half-type \
// RUN:   -fnative-half-arguments-and-returns | FileCheck -check-prefixes=FUN,SAFE %s

// RUN: %clang_cc1 -x hip %s -O3 -S -o - -triple=amdgcn-amd-amdhsa \
// RUN:   -fcuda-is-device -target-cpu gfx942 -fnative-half-type \
// RUN:   -fnative-half-arguments-and-returns -munsafe-fp-atomics \
// RUN:   | FileCheck -check-prefixes=FUN,UNSAFE %s

// REQUIRES: amdgpu-registered-target

#include "Inputs/cuda.h"
#include <stdatomic.h>

__global__ void ffp1(float *p) {
  // FUN-LABEL: @_Z4ffp1Pf
  // SAFEIR: atomicrmw fadd ptr {{.*}} monotonic, align 4, [[DEFMD:!amdgpu.no.fine.grained.memory ![0-9]+, !amdgpu.no.remote.memory ![0-9]+$]]
  // SAFEIR: atomicrmw fsub ptr {{.*}} monotonic, align 4, [[DEFMD]]
  // SAFEIR: atomicrmw fmax ptr {{.*}} monotonic, align 4, [[DEFMD]]
  // SAFEIR: atomicrmw fmin ptr {{.*}} monotonic, align 4, [[DEFMD]]
  // SAFEIR: atomicrmw fadd ptr {{.*}} syncscope("agent") monotonic, align 4, !noalias.addrspace ![[$NO_PRIVATE:[0-9]+]], [[DEFMD]]
  // SAFEIR: atomicrmw fsub ptr {{.*}} syncscope("workgroup") monotonic, align 4, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // SAFEIR: atomicrmw fmax ptr {{.*}} syncscope("agent") monotonic, align 4, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // SAFEIR: atomicrmw fmin ptr {{.*}} syncscope("workgroup") monotonic, align 4, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]

  // UNSAFEIR: atomicrmw fadd ptr {{.*}} monotonic, align 4, [[FADDMD:!amdgpu.no.fine.grained.memory ![0-9]+, !amdgpu.no.remote.memory ![0-9]+, !amdgpu.ignore.denormal.mode ![0-9]+$]]
  // UNSAFEIR: atomicrmw fsub ptr {{.*}} monotonic, align 4, [[DEFMD:!amdgpu.no.fine.grained.memory ![0-9]+, !amdgpu.no.remote.memory ![0-9]+$]]
  // UNSAFEIR: atomicrmw fmax ptr {{.*}} monotonic, align 4, [[DEFMD]]
  // UNSAFEIR: atomicrmw fmin ptr {{.*}} monotonic, align 4, [[DEFMD]]
  // UNSAFEIR: atomicrmw fadd ptr {{.*}} monotonic, align 4, !noalias.addrspace ![[$NO_PRIVATE:[0-9]+]], [[FADDMD]]
  // UNSAFEIR: atomicrmw fsub ptr {{.*}} monotonic, align 4, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // UNSAFEIR: atomicrmw fmax ptr {{.*}} syncscope("agent") monotonic, align 4, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // UNSAFEIR: atomicrmw fmin ptr {{.*}} syncscope("workgroup") monotonic, align 4, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]

  // SAFE: global_atomic_add_f32
  // SAFE: global_atomic_cmpswap
  // SAFE: global_atomic_max
  // SAFE: global_atomic_min
  // SAFE: global_atomic_max
  // SAFE: global_atomic_min

  // UNSAFE: global_atomic_add_f32
  // UNSAFE: global_atomic_cmpswap
  // UNSAFE: global_atomic_cmpswap
  // UNSAFE: global_atomic_cmpswap
  // UNSAFE: global_atomic_cmpswap
  // UNSAFE: global_atomic_cmpswap

  __atomic_fetch_add(p, 1.0f, memory_order_relaxed);
  __atomic_fetch_sub(p, 1.0f, memory_order_relaxed);
  __atomic_fetch_max(p, 1.0f, memory_order_relaxed);
  __atomic_fetch_min(p, 1.0f, memory_order_relaxed);

  __hip_atomic_fetch_add(p, 1.0f, memory_order_relaxed, __HIP_MEMORY_SCOPE_AGENT);
  __hip_atomic_fetch_sub(p, 1.0f, memory_order_relaxed, __HIP_MEMORY_SCOPE_WORKGROUP);
  __hip_atomic_fetch_max(p, 1.0f, memory_order_relaxed, __HIP_MEMORY_SCOPE_AGENT);
  __hip_atomic_fetch_min(p, 1.0f, memory_order_relaxed, __HIP_MEMORY_SCOPE_WORKGROUP);
}

__global__ void ffp2(double *p) {
  // FUN-LABEL: @_Z4ffp2Pd
  // SAFEIR: atomicrmw fadd ptr {{.*}} monotonic, align 8, [[DEFMD]]
  // SAFEIR: atomicrmw fsub ptr {{.*}} monotonic, align 8, [[DEFMD]]
  // SAFEIR: atomicrmw fmax ptr {{.*}} monotonic, align 8, [[DEFMD]]
  // SAFEIR: atomicrmw fmin ptr {{.*}} monotonic, align 8, [[DEFMD]]
  // SAFEIR: atomicrmw fadd ptr {{.*}} syncscope("agent") monotonic, align 8, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // SAFEIR: atomicrmw fsub ptr {{.*}} syncscope("workgroup") monotonic, align 8, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // SAFEIR: atomicrmw fmax ptr {{.*}} syncscope("agent") monotonic, align 8, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // SAFEIR: atomicrmw fmin ptr {{.*}} syncscope("workgroup") monotonic, align 8, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]

  // UNSAFEIR: atomicrmw fadd ptr {{.*}} monotonic, align 8, [[DEFMD]]
  // UNSAFEIR: atomicrmw fsub ptr {{.*}} monotonic, align 8, [[DEFMD]]
  // UNSAFEIR: atomicrmw fmax ptr {{.*}} monotonic, align 8, [[DEFMD]]
  // UNSAFEIR: atomicrmw fmin ptr {{.*}} monotonic, align 8, [[DEFMD]]
  // UNSAFEIR: atomicrmw fadd ptr {{.*}} syncscope("agent") monotonic, align 8, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // UNSAFEIR: atomicrmw fsub ptr {{.*}} syncscope("workgroup") monotonic, align 8, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // UNSAFEIR: atomicrmw fmax ptr {{.*}} syncscope("agent") monotonic, align 8, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // UNSAFEIR: atomicrmw fmin ptr {{.*}} syncscope("workgroup") monotonic, align 8, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]

  // SAFE: global_atomic_cmpswap_b64
  // SAFE: global_atomic_cmpswap_b64
  // SAFE: global_atomic_cmpswap_b64
  // SAFE: global_atomic_cmpswap_b64
  // SAFE: global_atomic_cmpswap_b64
  // SAFE: global_atomic_cmpswap_b64

  // UNSAFE: global_atomic_add_f64
  // UNSAFE: global_atomic_cmpswap_x2
  // UNSAFE: global_atomic_max_f64
  // UNSAFE: global_atomic_min_f64
  // UNSAFE: global_atomic_max_f64
  // UNSAFE: global_atomic_min_f64
  __atomic_fetch_add(p, 1.0, memory_order_relaxed);
  __atomic_fetch_sub(p, 1.0, memory_order_relaxed);
  __atomic_fetch_max(p, 1.0, memory_order_relaxed);
  __atomic_fetch_min(p, 1.0, memory_order_relaxed);
  __hip_atomic_fetch_add(p, 1.0, memory_order_relaxed, __HIP_MEMORY_SCOPE_AGENT);
  __hip_atomic_fetch_sub(p, 1.0, memory_order_relaxed, __HIP_MEMORY_SCOPE_WORKGROUP);
  __hip_atomic_fetch_max(p, 1.0, memory_order_relaxed, __HIP_MEMORY_SCOPE_AGENT);
  __hip_atomic_fetch_min(p, 1.0, memory_order_relaxed, __HIP_MEMORY_SCOPE_WORKGROUP);
}

// long double is the same as double for amdgcn.
__global__ void ffp3(long double *p) {
  // FUN-LABEL: @_Z4ffp3Pe
  // SAFEIR: atomicrmw fadd ptr {{.*}} monotonic, align 8, [[DEFMD]]
  // SAFEIR: atomicrmw fsub ptr {{.*}} monotonic, align 8, [[DEFMD]]
  // SAFEIR: atomicrmw fmax ptr {{.*}} monotonic, align 8, [[DEFMD]]
  // SAFEIR: atomicrmw fmin ptr {{.*}} monotonic, align 8, [[DEFMD]]
  // SAFEIR: atomicrmw fadd ptr {{.*}} syncscope("agent") monotonic, align 8, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // SAFEIR: atomicrmw fsub ptr {{.*}} syncscope("workgroup") monotonic, align 8, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // SAFEIR: atomicrmw fmax ptr {{.*}} syncscope("agent") monotonic, align 8, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // SAFEIR: atomicrmw fmin ptr {{.*}} syncscope("workgroup") monotonic, align 8, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]

  // UNSAFEIR: atomicrmw fadd ptr {{.*}} monotonic, align 8, [[DEFMD]]
  // UNSAFEIR: atomicrmw fsub ptr {{.*}} monotonic, align 8, [[DEFMD]]
  // UNSAFEIR: atomicrmw fmax ptr {{.*}} monotonic, align 8, [[DEFMD]]
  // UNSAFEIR: atomicrmw fmin ptr {{.*}} monotonic, align 8, [[DEFMD]]
  // UNSAFEIR: atomicrmw fadd ptr {{.*}} syncscope("agent") monotonic, align 8, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // UNSAFEIR: atomicrmw fsub ptr {{.*}} syncscope("workgroup") monotonic, align 8, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // UNSAFEIR: atomicrmw fmax ptr {{.*}} syncscope("agent") monotonic, align 8, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // UNSAFEIR: atomicrmw fmin ptr {{.*}} syncscope("workgroup") monotonic, align 8, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]

  // SAFE: global_atomic_cmpswap_b64
  // SAFE: global_atomic_cmpswap_b64
  // SAFE: global_atomic_cmpswap_b64
  // SAFE: global_atomic_cmpswap_b64
  // SAFE: global_atomic_cmpswap_b64

  // UNSAFE: global_atomic_cmpswap_x2
  // UNSAFE: global_atomic_max_f64
  // UNSAFE: global_atomic_min_f64
  // UNSAFE: global_atomic_max_f64
  // UNSAFE: global_atomic_min_f64
  __atomic_fetch_add(p, 1.0L, memory_order_relaxed);
  __atomic_fetch_sub(p, 1.0L, memory_order_relaxed);
  __atomic_fetch_max(p, 1.0L, memory_order_relaxed);
  __atomic_fetch_min(p, 1.0L, memory_order_relaxed);
  __hip_atomic_fetch_add(p, 1.0L, memory_order_relaxed, __HIP_MEMORY_SCOPE_AGENT);
  __hip_atomic_fetch_sub(p, 1.0L, memory_order_relaxed, __HIP_MEMORY_SCOPE_WORKGROUP);
  __hip_atomic_fetch_max(p, 1.0L, memory_order_relaxed, __HIP_MEMORY_SCOPE_AGENT);
  __hip_atomic_fetch_min(p, 1.0L, memory_order_relaxed, __HIP_MEMORY_SCOPE_WORKGROUP);
}

__device__ double ffp4(double *p, float f) {
  // FUN-LABEL: @_Z4ffp4Pdf
  // CHECK: fpext contract float {{.*}} to double
  // SAFEIR: atomicrmw fsub ptr {{.*}} monotonic, align 8, [[DEFMD]]
  // UNSAFEIR: atomicrmw fsub ptr {{.*}} monotonic, align 8, [[DEFMD]]

  // SAFEIR: atomicrmw fsub ptr {{.*}} monotonic, align 8, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // UNSAFEIR: atomicrmw fsub ptr {{.*}} monotonic, align 8, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  __atomic_fetch_sub(p, f, memory_order_relaxed);
  return __hip_atomic_fetch_sub(p, f, memory_order_relaxed, __HIP_MEMORY_SCOPE_AGENT);
}

__device__ double ffp5(double *p, int i) {
  // FUN-LABEL: @_Z4ffp5Pdi
  // CHECK: sitofp i32 {{.*}} to double
  // SAFEIR: atomicrmw fsub ptr {{.*}} monotonic, align 8, [[DEFMD]]
  // UNSAFEIR: atomicrmw fsub ptr {{.*}} monotonic, align 8, [[DEFMD]]
  __atomic_fetch_sub(p, i, memory_order_relaxed);

  // SAFEIR: atomicrmw fsub ptr {{.*}} monotonic, align 8, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // UNSAFEIR: atomicrmw fsub ptr {{.*}} monotonic, align 8, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  return __hip_atomic_fetch_sub(p, i, memory_order_relaxed, __HIP_MEMORY_SCOPE_AGENT);
}

__global__ void ffp6(_Float16 *p) {
  // FUN-LABEL: @_Z4ffp6PDF16
  // SAFEIR: atomicrmw fadd ptr {{.*}} monotonic, align 2, [[DEFMD]]
  // SAFEIR: atomicrmw fsub ptr {{.*}} monotonic, align 2, [[DEFMD]]
  // SAFEIR: atomicrmw fmax ptr {{.*}} monotonic, align 2, [[DEFMD]]
  // SAFEIR: atomicrmw fmin ptr {{.*}} monotonic, align 2, [[DEFMD]]
  // SAFEIR: atomicrmw fadd ptr {{.*}} syncscope("agent") monotonic, align 2, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // SAFEIR: atomicrmw fsub ptr {{.*}} syncscope("workgroup") monotonic, align 2, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // SAFEIR: atomicrmw fmax ptr {{.*}} syncscope("agent") monotonic, align 2, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // SAFEIR: atomicrmw fmin ptr {{.*}} syncscope("workgroup") monotonic, align 2, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]

  // UNSAFEIR: atomicrmw fadd ptr {{.*}} monotonic, align 2, [[DEFMD]]
  // UNSAFEIR: atomicrmw fsub ptr {{.*}} monotonic, align 2, [[DEFMD]]
  // UNSAFEIR: atomicrmw fmax ptr {{.*}} monotonic, align 2, [[DEFMD]]
  // UNSAFEIR: atomicrmw fmin ptr {{.*}} monotonic, align 2, [[DEFMD]]
  // UNSAFEIR: atomicrmw fadd ptr {{.*}} monotonic, align 2, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // UNSAFEIR: atomicrmw fsub ptr {{.*}} monotonic, align 2, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // UNSAFEIR: atomicrmw fmax ptr {{.*}} syncscope("agent") monotonic, align 2, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]
  // UNSAFEIR: atomicrmw fmin ptr {{.*}} syncscope("workgroup") monotonic, align 2, !noalias.addrspace ![[$NO_PRIVATE]], [[DEFMD]]

  // SAFE: _Z4ffp6PDF16
  // SAFE: global_atomic_cmpswap
  // SAFE: global_atomic_cmpswap
  // SAFE: global_atomic_cmpswap
  // SAFE: global_atomic_cmpswap
  // SAFE: global_atomic_cmpswap
  // SAFE: global_atomic_cmpswap

  // UNSAFE: _Z4ffp6PDF16
  // UNSAFE: global_atomic_cmpswap
  // UNSAFE: global_atomic_cmpswap
  // UNSAFE: global_atomic_cmpswap
  // UNSAFE: global_atomic_cmpswap
  // UNSAFE: global_atomic_cmpswap
  // UNSAFE: global_atomic_cmpswap
  __atomic_fetch_add(p, 1.0, memory_order_relaxed);
  __atomic_fetch_sub(p, 1.0, memory_order_relaxed);
  __atomic_fetch_max(p, 1.0, memory_order_relaxed);
  __atomic_fetch_min(p, 1.0, memory_order_relaxed);

  __hip_atomic_fetch_add(p, 1.0f, memory_order_relaxed, __HIP_MEMORY_SCOPE_AGENT);
  __hip_atomic_fetch_sub(p, 1.0f, memory_order_relaxed, __HIP_MEMORY_SCOPE_WORKGROUP);
  __hip_atomic_fetch_max(p, 1.0f, memory_order_relaxed, __HIP_MEMORY_SCOPE_AGENT);
  __hip_atomic_fetch_min(p, 1.0f, memory_order_relaxed, __HIP_MEMORY_SCOPE_WORKGROUP);
}

// CHECK-LABEL: @_Z12test_cmpxchgPiii
// CHECK: cmpxchg ptr %{{.+}}, i32 %{{.+}}, i32 %{{.+}} acquire acquire, align 4{{$}}
// CHECK: cmpxchg weak ptr %{{.+}}, i32 %{{.+}}, i32 %{{.+}} acquire acquire, align 4{{$}}
// CHECK: cmpxchg ptr %{{.+}}, i32 %{{.+}}, i32 %{{.+}} syncscope("workgroup") monotonic monotonic, align 4, !noalias.addrspace ![[$NO_PRIVATE]]{{$}}
// CHECK: cmpxchg weak ptr %{{.+}}, i32 %{{.+}}, i32 %{{.+}} syncscope("workgroup") monotonic monotonic, align 4, !noalias.addrspace ![[$NO_PRIVATE]]{{$}}
__device__ int test_cmpxchg(int *ptr, int cmp, int desired) {
  bool flag = __atomic_compare_exchange(ptr, &cmp, &desired, 0, memory_order_acquire, memory_order_acquire);
  flag = __atomic_compare_exchange_n(ptr, &cmp, desired, 1, memory_order_acquire, memory_order_acquire);
  flag = __hip_atomic_compare_exchange_strong(ptr, &cmp, desired, __ATOMIC_RELAXED, __ATOMIC_RELAXED, __HIP_MEMORY_SCOPE_WORKGROUP);
  flag = __hip_atomic_compare_exchange_weak(ptr, &cmp, desired, __ATOMIC_RELAXED, __ATOMIC_RELAXED, __HIP_MEMORY_SCOPE_WORKGROUP);
  return flag;
}

// SAFEIR: ![[$NO_PRIVATE]] = !{i32 5, i32 6}
// UNSAFEIR: ![[$NO_PRIVATE]] = !{i32 5, i32 6}

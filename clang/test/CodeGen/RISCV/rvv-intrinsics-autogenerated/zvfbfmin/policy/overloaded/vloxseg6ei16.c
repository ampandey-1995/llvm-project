// NOTE: Assertions have been autogenerated by utils/update_cc_test_checks.py UTC_ARGS: --version 4
// REQUIRES: riscv-registered-target
// RUN: %clang_cc1 -triple riscv64 -target-feature +zve64x \
// RUN:   -target-feature +zvfbfmin -disable-O0-optnone \
// RUN:   -emit-llvm %s -o - | opt -S -passes=mem2reg | \
// RUN:   FileCheck --check-prefix=CHECK-RV64 %s

#include <riscv_vector.h>

// CHECK-RV64-LABEL: define dso_local target("riscv.vector.tuple", <vscale x 2 x i8>, 6) @test_vloxseg6ei16_v_bf16mf4x6_tu(
// CHECK-RV64-SAME: target("riscv.vector.tuple", <vscale x 2 x i8>, 6) [[VD:%.*]], ptr noundef [[RS1:%.*]], <vscale x 1 x i16> [[RS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0:[0-9]+]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call target("riscv.vector.tuple", <vscale x 2 x i8>, 6) @llvm.riscv.vloxseg6.triscv.vector.tuple_nxv2i8_6t.p0.nxv1i16.i64(target("riscv.vector.tuple", <vscale x 2 x i8>, 6) [[VD]], ptr [[RS1]], <vscale x 1 x i16> [[RS2]], i64 [[VL]], i64 4)
// CHECK-RV64-NEXT:    ret target("riscv.vector.tuple", <vscale x 2 x i8>, 6) [[TMP0]]
//
vbfloat16mf4x6_t test_vloxseg6ei16_v_bf16mf4x6_tu(vbfloat16mf4x6_t vd,
                                                  const __bf16 *rs1,
                                                  vuint16mf4_t rs2, size_t vl) {
  return __riscv_vloxseg6ei16_tu(vd, rs1, rs2, vl);
}

// CHECK-RV64-LABEL: define dso_local target("riscv.vector.tuple", <vscale x 4 x i8>, 6) @test_vloxseg6ei16_v_bf16mf2x6_tu(
// CHECK-RV64-SAME: target("riscv.vector.tuple", <vscale x 4 x i8>, 6) [[VD:%.*]], ptr noundef [[RS1:%.*]], <vscale x 2 x i16> [[RS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call target("riscv.vector.tuple", <vscale x 4 x i8>, 6) @llvm.riscv.vloxseg6.triscv.vector.tuple_nxv4i8_6t.p0.nxv2i16.i64(target("riscv.vector.tuple", <vscale x 4 x i8>, 6) [[VD]], ptr [[RS1]], <vscale x 2 x i16> [[RS2]], i64 [[VL]], i64 4)
// CHECK-RV64-NEXT:    ret target("riscv.vector.tuple", <vscale x 4 x i8>, 6) [[TMP0]]
//
vbfloat16mf2x6_t test_vloxseg6ei16_v_bf16mf2x6_tu(vbfloat16mf2x6_t vd,
                                                  const __bf16 *rs1,
                                                  vuint16mf2_t rs2, size_t vl) {
  return __riscv_vloxseg6ei16_tu(vd, rs1, rs2, vl);
}

// CHECK-RV64-LABEL: define dso_local target("riscv.vector.tuple", <vscale x 8 x i8>, 6) @test_vloxseg6ei16_v_bf16m1x6_tu(
// CHECK-RV64-SAME: target("riscv.vector.tuple", <vscale x 8 x i8>, 6) [[VD:%.*]], ptr noundef [[RS1:%.*]], <vscale x 4 x i16> [[RS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call target("riscv.vector.tuple", <vscale x 8 x i8>, 6) @llvm.riscv.vloxseg6.triscv.vector.tuple_nxv8i8_6t.p0.nxv4i16.i64(target("riscv.vector.tuple", <vscale x 8 x i8>, 6) [[VD]], ptr [[RS1]], <vscale x 4 x i16> [[RS2]], i64 [[VL]], i64 4)
// CHECK-RV64-NEXT:    ret target("riscv.vector.tuple", <vscale x 8 x i8>, 6) [[TMP0]]
//
vbfloat16m1x6_t test_vloxseg6ei16_v_bf16m1x6_tu(vbfloat16m1x6_t vd,
                                                const __bf16 *rs1,
                                                vuint16m1_t rs2, size_t vl) {
  return __riscv_vloxseg6ei16_tu(vd, rs1, rs2, vl);
}

// CHECK-RV64-LABEL: define dso_local target("riscv.vector.tuple", <vscale x 2 x i8>, 6) @test_vloxseg6ei16_v_bf16mf4x6_tum(
// CHECK-RV64-SAME: <vscale x 1 x i1> [[VM:%.*]], target("riscv.vector.tuple", <vscale x 2 x i8>, 6) [[VD:%.*]], ptr noundef [[RS1:%.*]], <vscale x 1 x i16> [[RS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call target("riscv.vector.tuple", <vscale x 2 x i8>, 6) @llvm.riscv.vloxseg6.mask.triscv.vector.tuple_nxv2i8_6t.p0.nxv1i16.nxv1i1.i64(target("riscv.vector.tuple", <vscale x 2 x i8>, 6) [[VD]], ptr [[RS1]], <vscale x 1 x i16> [[RS2]], <vscale x 1 x i1> [[VM]], i64 [[VL]], i64 2, i64 4)
// CHECK-RV64-NEXT:    ret target("riscv.vector.tuple", <vscale x 2 x i8>, 6) [[TMP0]]
//
vbfloat16mf4x6_t test_vloxseg6ei16_v_bf16mf4x6_tum(vbool64_t vm,
                                                   vbfloat16mf4x6_t vd,
                                                   const __bf16 *rs1,
                                                   vuint16mf4_t rs2,
                                                   size_t vl) {
  return __riscv_vloxseg6ei16_tum(vm, vd, rs1, rs2, vl);
}

// CHECK-RV64-LABEL: define dso_local target("riscv.vector.tuple", <vscale x 4 x i8>, 6) @test_vloxseg6ei16_v_bf16mf2x6_tum(
// CHECK-RV64-SAME: <vscale x 2 x i1> [[VM:%.*]], target("riscv.vector.tuple", <vscale x 4 x i8>, 6) [[VD:%.*]], ptr noundef [[RS1:%.*]], <vscale x 2 x i16> [[RS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call target("riscv.vector.tuple", <vscale x 4 x i8>, 6) @llvm.riscv.vloxseg6.mask.triscv.vector.tuple_nxv4i8_6t.p0.nxv2i16.nxv2i1.i64(target("riscv.vector.tuple", <vscale x 4 x i8>, 6) [[VD]], ptr [[RS1]], <vscale x 2 x i16> [[RS2]], <vscale x 2 x i1> [[VM]], i64 [[VL]], i64 2, i64 4)
// CHECK-RV64-NEXT:    ret target("riscv.vector.tuple", <vscale x 4 x i8>, 6) [[TMP0]]
//
vbfloat16mf2x6_t test_vloxseg6ei16_v_bf16mf2x6_tum(vbool32_t vm,
                                                   vbfloat16mf2x6_t vd,
                                                   const __bf16 *rs1,
                                                   vuint16mf2_t rs2,
                                                   size_t vl) {
  return __riscv_vloxseg6ei16_tum(vm, vd, rs1, rs2, vl);
}

// CHECK-RV64-LABEL: define dso_local target("riscv.vector.tuple", <vscale x 8 x i8>, 6) @test_vloxseg6ei16_v_bf16m1x6_tum(
// CHECK-RV64-SAME: <vscale x 4 x i1> [[VM:%.*]], target("riscv.vector.tuple", <vscale x 8 x i8>, 6) [[VD:%.*]], ptr noundef [[RS1:%.*]], <vscale x 4 x i16> [[RS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call target("riscv.vector.tuple", <vscale x 8 x i8>, 6) @llvm.riscv.vloxseg6.mask.triscv.vector.tuple_nxv8i8_6t.p0.nxv4i16.nxv4i1.i64(target("riscv.vector.tuple", <vscale x 8 x i8>, 6) [[VD]], ptr [[RS1]], <vscale x 4 x i16> [[RS2]], <vscale x 4 x i1> [[VM]], i64 [[VL]], i64 2, i64 4)
// CHECK-RV64-NEXT:    ret target("riscv.vector.tuple", <vscale x 8 x i8>, 6) [[TMP0]]
//
vbfloat16m1x6_t test_vloxseg6ei16_v_bf16m1x6_tum(vbool16_t vm,
                                                 vbfloat16m1x6_t vd,
                                                 const __bf16 *rs1,
                                                 vuint16m1_t rs2, size_t vl) {
  return __riscv_vloxseg6ei16_tum(vm, vd, rs1, rs2, vl);
}

// CHECK-RV64-LABEL: define dso_local target("riscv.vector.tuple", <vscale x 2 x i8>, 6) @test_vloxseg6ei16_v_bf16mf4x6_tumu(
// CHECK-RV64-SAME: <vscale x 1 x i1> [[VM:%.*]], target("riscv.vector.tuple", <vscale x 2 x i8>, 6) [[VD:%.*]], ptr noundef [[RS1:%.*]], <vscale x 1 x i16> [[RS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call target("riscv.vector.tuple", <vscale x 2 x i8>, 6) @llvm.riscv.vloxseg6.mask.triscv.vector.tuple_nxv2i8_6t.p0.nxv1i16.nxv1i1.i64(target("riscv.vector.tuple", <vscale x 2 x i8>, 6) [[VD]], ptr [[RS1]], <vscale x 1 x i16> [[RS2]], <vscale x 1 x i1> [[VM]], i64 [[VL]], i64 0, i64 4)
// CHECK-RV64-NEXT:    ret target("riscv.vector.tuple", <vscale x 2 x i8>, 6) [[TMP0]]
//
vbfloat16mf4x6_t test_vloxseg6ei16_v_bf16mf4x6_tumu(vbool64_t vm,
                                                    vbfloat16mf4x6_t vd,
                                                    const __bf16 *rs1,
                                                    vuint16mf4_t rs2,
                                                    size_t vl) {
  return __riscv_vloxseg6ei16_tumu(vm, vd, rs1, rs2, vl);
}

// CHECK-RV64-LABEL: define dso_local target("riscv.vector.tuple", <vscale x 4 x i8>, 6) @test_vloxseg6ei16_v_bf16mf2x6_tumu(
// CHECK-RV64-SAME: <vscale x 2 x i1> [[VM:%.*]], target("riscv.vector.tuple", <vscale x 4 x i8>, 6) [[VD:%.*]], ptr noundef [[RS1:%.*]], <vscale x 2 x i16> [[RS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call target("riscv.vector.tuple", <vscale x 4 x i8>, 6) @llvm.riscv.vloxseg6.mask.triscv.vector.tuple_nxv4i8_6t.p0.nxv2i16.nxv2i1.i64(target("riscv.vector.tuple", <vscale x 4 x i8>, 6) [[VD]], ptr [[RS1]], <vscale x 2 x i16> [[RS2]], <vscale x 2 x i1> [[VM]], i64 [[VL]], i64 0, i64 4)
// CHECK-RV64-NEXT:    ret target("riscv.vector.tuple", <vscale x 4 x i8>, 6) [[TMP0]]
//
vbfloat16mf2x6_t test_vloxseg6ei16_v_bf16mf2x6_tumu(vbool32_t vm,
                                                    vbfloat16mf2x6_t vd,
                                                    const __bf16 *rs1,
                                                    vuint16mf2_t rs2,
                                                    size_t vl) {
  return __riscv_vloxseg6ei16_tumu(vm, vd, rs1, rs2, vl);
}

// CHECK-RV64-LABEL: define dso_local target("riscv.vector.tuple", <vscale x 8 x i8>, 6) @test_vloxseg6ei16_v_bf16m1x6_tumu(
// CHECK-RV64-SAME: <vscale x 4 x i1> [[VM:%.*]], target("riscv.vector.tuple", <vscale x 8 x i8>, 6) [[VD:%.*]], ptr noundef [[RS1:%.*]], <vscale x 4 x i16> [[RS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call target("riscv.vector.tuple", <vscale x 8 x i8>, 6) @llvm.riscv.vloxseg6.mask.triscv.vector.tuple_nxv8i8_6t.p0.nxv4i16.nxv4i1.i64(target("riscv.vector.tuple", <vscale x 8 x i8>, 6) [[VD]], ptr [[RS1]], <vscale x 4 x i16> [[RS2]], <vscale x 4 x i1> [[VM]], i64 [[VL]], i64 0, i64 4)
// CHECK-RV64-NEXT:    ret target("riscv.vector.tuple", <vscale x 8 x i8>, 6) [[TMP0]]
//
vbfloat16m1x6_t test_vloxseg6ei16_v_bf16m1x6_tumu(vbool16_t vm,
                                                  vbfloat16m1x6_t vd,
                                                  const __bf16 *rs1,
                                                  vuint16m1_t rs2, size_t vl) {
  return __riscv_vloxseg6ei16_tumu(vm, vd, rs1, rs2, vl);
}

// CHECK-RV64-LABEL: define dso_local target("riscv.vector.tuple", <vscale x 2 x i8>, 6) @test_vloxseg6ei16_v_bf16mf4x6_mu(
// CHECK-RV64-SAME: <vscale x 1 x i1> [[VM:%.*]], target("riscv.vector.tuple", <vscale x 2 x i8>, 6) [[VD:%.*]], ptr noundef [[RS1:%.*]], <vscale x 1 x i16> [[RS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call target("riscv.vector.tuple", <vscale x 2 x i8>, 6) @llvm.riscv.vloxseg6.mask.triscv.vector.tuple_nxv2i8_6t.p0.nxv1i16.nxv1i1.i64(target("riscv.vector.tuple", <vscale x 2 x i8>, 6) [[VD]], ptr [[RS1]], <vscale x 1 x i16> [[RS2]], <vscale x 1 x i1> [[VM]], i64 [[VL]], i64 1, i64 4)
// CHECK-RV64-NEXT:    ret target("riscv.vector.tuple", <vscale x 2 x i8>, 6) [[TMP0]]
//
vbfloat16mf4x6_t test_vloxseg6ei16_v_bf16mf4x6_mu(vbool64_t vm,
                                                  vbfloat16mf4x6_t vd,
                                                  const __bf16 *rs1,
                                                  vuint16mf4_t rs2, size_t vl) {
  return __riscv_vloxseg6ei16_mu(vm, vd, rs1, rs2, vl);
}

// CHECK-RV64-LABEL: define dso_local target("riscv.vector.tuple", <vscale x 4 x i8>, 6) @test_vloxseg6ei16_v_bf16mf2x6_mu(
// CHECK-RV64-SAME: <vscale x 2 x i1> [[VM:%.*]], target("riscv.vector.tuple", <vscale x 4 x i8>, 6) [[VD:%.*]], ptr noundef [[RS1:%.*]], <vscale x 2 x i16> [[RS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call target("riscv.vector.tuple", <vscale x 4 x i8>, 6) @llvm.riscv.vloxseg6.mask.triscv.vector.tuple_nxv4i8_6t.p0.nxv2i16.nxv2i1.i64(target("riscv.vector.tuple", <vscale x 4 x i8>, 6) [[VD]], ptr [[RS1]], <vscale x 2 x i16> [[RS2]], <vscale x 2 x i1> [[VM]], i64 [[VL]], i64 1, i64 4)
// CHECK-RV64-NEXT:    ret target("riscv.vector.tuple", <vscale x 4 x i8>, 6) [[TMP0]]
//
vbfloat16mf2x6_t test_vloxseg6ei16_v_bf16mf2x6_mu(vbool32_t vm,
                                                  vbfloat16mf2x6_t vd,
                                                  const __bf16 *rs1,
                                                  vuint16mf2_t rs2, size_t vl) {
  return __riscv_vloxseg6ei16_mu(vm, vd, rs1, rs2, vl);
}

// CHECK-RV64-LABEL: define dso_local target("riscv.vector.tuple", <vscale x 8 x i8>, 6) @test_vloxseg6ei16_v_bf16m1x6_mu(
// CHECK-RV64-SAME: <vscale x 4 x i1> [[VM:%.*]], target("riscv.vector.tuple", <vscale x 8 x i8>, 6) [[VD:%.*]], ptr noundef [[RS1:%.*]], <vscale x 4 x i16> [[RS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call target("riscv.vector.tuple", <vscale x 8 x i8>, 6) @llvm.riscv.vloxseg6.mask.triscv.vector.tuple_nxv8i8_6t.p0.nxv4i16.nxv4i1.i64(target("riscv.vector.tuple", <vscale x 8 x i8>, 6) [[VD]], ptr [[RS1]], <vscale x 4 x i16> [[RS2]], <vscale x 4 x i1> [[VM]], i64 [[VL]], i64 1, i64 4)
// CHECK-RV64-NEXT:    ret target("riscv.vector.tuple", <vscale x 8 x i8>, 6) [[TMP0]]
//
vbfloat16m1x6_t test_vloxseg6ei16_v_bf16m1x6_mu(vbool16_t vm,
                                                vbfloat16m1x6_t vd,
                                                const __bf16 *rs1,
                                                vuint16m1_t rs2, size_t vl) {
  return __riscv_vloxseg6ei16_mu(vm, vd, rs1, rs2, vl);
}

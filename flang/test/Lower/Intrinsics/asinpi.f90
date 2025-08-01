! RUN: bbc -emit-fir -hlfir=false %s -o - | FileCheck %s --check-prefixes="CHECK,CHECK-FAST"
! RUN: bbc --math-runtime=precise -emit-fir -hlfir=false %s -o - | FileCheck %s --check-prefixes="CHECK,CHECK-PRECISE"
! RUN: %flang_fc1 -emit-fir -flang-deprecated-no-hlfir %s -o - | FileCheck %s --check-prefixes="CHECK,CHECK-FAST"

function test_real4(x)
  real :: x, test_real4
  test_real4 = asinpi(x)
end function

! CHECK-LABEL: @_QPtest_real4
! CHECK-PRECISE: %[[asin:.*]] = fir.call @asinf({{%[A-Za-z0-9._]+}}) fastmath<contract> : (f32) -> f32
! CHECK-FAST: %[[asin:.*]] = math.asin %{{.*}} : f32
! CHECK: %[[dpi:.*]] = arith.constant 0.31830988618379069 : f64
! CHECK: %[[inv_pi:.*]] = fir.convert %[[dpi]] : (f64) -> f32
! CHECK: %{{.*}} = arith.mulf %[[asin]], %[[inv_pi]] fastmath<contract> : f32

function test_real8(x)
  real(8) :: x, test_real8
  test_real8 = asinpi(x)
end function

! CHECK-LABEL: @_QPtest_real8
! CHECK-PRECISE: %[[asin:.*]] = fir.call @asin({{%[A-Za-z0-9._]+}}) fastmath<contract> : (f64) -> f64
! CHECK-FAST: %[[asin:.*]] = math.asin %{{.*}} : f64
! CHECK: %[[inv_pi:.*]] = arith.constant 0.31830988618379069 : f64
! CHECK: %{{.*}} = arith.mulf %[[asin]], %[[inv_pi]] fastmath<contract> : f64

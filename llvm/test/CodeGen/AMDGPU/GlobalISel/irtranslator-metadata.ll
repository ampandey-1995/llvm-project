; NOTE: Assertions have been autogenerated by utils/update_mir_test_checks.py
; RUN: llc -global-isel -mtriple=amdgcn -mcpu=gfx900 -stop-after=irtranslator -o - %s | FileCheck %s
; Make sure intrinsics with metadata arguments are translated

define i32 @reloc_constant() {
  ; CHECK-LABEL: name: reloc_constant
  ; CHECK: bb.1 (%ir-block.0):
  ; CHECK-NEXT:   [[INT:%[0-9]+]]:_(s32) = G_INTRINSIC intrinsic(@llvm.amdgcn.reloc.constant), !1
  ; CHECK-NEXT:   [[INT1:%[0-9]+]]:_(s32) = G_INTRINSIC intrinsic(@llvm.amdgcn.reloc.constant), <0x{{[0-9a-f]+}}>
  ; CHECK-NEXT:   [[ADD:%[0-9]+]]:_(s32) = G_ADD [[INT]], [[INT1]]
  ; CHECK-NEXT:   $vgpr0 = COPY [[ADD]](s32)
  ; CHECK-NEXT:   SI_RETURN implicit $vgpr0
  %val0 = call i32 @llvm.amdgcn.reloc.constant(metadata !0)
  %val1 = call i32 @llvm.amdgcn.reloc.constant(metadata i32 4)
  %res = add i32 %val0, %val1
  ret i32 %res
}

declare i32 @llvm.amdgcn.reloc.constant(metadata) #0

attributes #0 = { nounwind readnone speculatable willreturn }

!0 = !{!"arst"}

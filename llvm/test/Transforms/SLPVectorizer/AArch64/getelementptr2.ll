; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
;test_i16_extend NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt -S -mtriple=aarch64--linux-gnu -passes=slp-vectorizer -slp-threshold=-5 -pass-remarks-output=%t < %s | FileCheck %s
; RUN: cat %t | FileCheck -check-prefix=YAML %s
; RUN: opt -S -mtriple=aarch64--linux-gnu -passes=slp-vectorizer -slp-threshold=-5 -pass-remarks-output=%t < %s | FileCheck %s
; RUN: cat %t | FileCheck -check-prefix=YAML %s


@global = internal global { ptr } zeroinitializer, align 8

; YAML-LABEL: --- !Passed
; YAML-NEXT:  Pass:            slp-vectorizer
; YAML-NEXT:  Name:            VectorizedList
; YAML-NEXT:  Function:        test_i16_extend
; YAML-NEXT:  Args:
; YAML-NEXT:    - String:          'SLP vectorized with cost '
; YAML-NEXT:    - Cost:            '-16
; YAML-NEXT:    - String:          ' and with tree size '
; YAML-NEXT:    - TreeSize:        '5'
; YAML-NEXT:  ...

; Make sure we vectorize to maximize the load with when loading i16 and
; extending it for compute operations.
define void @test_i16_extend(ptr %p.1, ptr %p.2, i32 %idx.i32) {
; CHECK-LABEL: @test_i16_extend(
; CHECK-NEXT:    [[P_0:%.*]] = load ptr, ptr @global, align 8
; CHECK-NEXT:    [[IDX_0:%.*]] = zext i32 [[IDX_I32:%.*]] to i64
; CHECK-NEXT:    [[T53:%.*]] = getelementptr inbounds i16, ptr [[P_1:%.*]], i64 [[IDX_0]]
; CHECK-NEXT:    [[T56:%.*]] = getelementptr inbounds i16, ptr [[P_2:%.*]], i64 [[IDX_0]]
; CHECK-NEXT:    [[TMP1:%.*]] = load <8 x i16>, ptr [[T53]], align 2
; CHECK-NEXT:    [[TMP2:%.*]] = zext <8 x i16> [[TMP1]] to <8 x i32>
; CHECK-NEXT:    [[TMP3:%.*]] = load <8 x i16>, ptr [[T56]], align 2
; CHECK-NEXT:    [[TMP4:%.*]] = zext <8 x i16> [[TMP3]] to <8 x i32>
; CHECK-NEXT:    [[TMP5:%.*]] = sub <8 x i32> [[TMP2]], [[TMP4]]
; CHECK-NEXT:    [[TMP6:%.*]] = extractelement <8 x i32> [[TMP5]], i32 0
; CHECK-NEXT:    [[TMP7:%.*]] = sext i32 [[TMP6]] to i64
; CHECK-NEXT:    [[T60:%.*]] = getelementptr inbounds i32, ptr [[P_0]], i64 [[TMP7]]
; CHECK-NEXT:    [[L_1:%.*]] = load i32, ptr [[T60]], align 4
; CHECK-NEXT:    [[TMP8:%.*]] = extractelement <8 x i32> [[TMP5]], i32 1
; CHECK-NEXT:    [[TMP9:%.*]] = sext i32 [[TMP8]] to i64
; CHECK-NEXT:    [[T71:%.*]] = getelementptr inbounds i32, ptr [[P_0]], i64 [[TMP9]]
; CHECK-NEXT:    [[L_2:%.*]] = load i32, ptr [[T71]], align 4
; CHECK-NEXT:    [[TMP10:%.*]] = extractelement <8 x i32> [[TMP5]], i32 2
; CHECK-NEXT:    [[TMP11:%.*]] = sext i32 [[TMP10]] to i64
; CHECK-NEXT:    [[T82:%.*]] = getelementptr inbounds i32, ptr [[P_0]], i64 [[TMP11]]
; CHECK-NEXT:    [[L_3:%.*]] = load i32, ptr [[T82]], align 4
; CHECK-NEXT:    [[TMP12:%.*]] = extractelement <8 x i32> [[TMP5]], i32 3
; CHECK-NEXT:    [[TMP13:%.*]] = sext i32 [[TMP12]] to i64
; CHECK-NEXT:    [[T93:%.*]] = getelementptr inbounds i32, ptr [[P_0]], i64 [[TMP13]]
; CHECK-NEXT:    [[L_4:%.*]] = load i32, ptr [[T93]], align 4
; CHECK-NEXT:    [[TMP14:%.*]] = extractelement <8 x i32> [[TMP5]], i32 4
; CHECK-NEXT:    [[TMP15:%.*]] = sext i32 [[TMP14]] to i64
; CHECK-NEXT:    [[T104:%.*]] = getelementptr inbounds i32, ptr [[P_0]], i64 [[TMP15]]
; CHECK-NEXT:    [[L_5:%.*]] = load i32, ptr [[T104]], align 4
; CHECK-NEXT:    [[TMP16:%.*]] = extractelement <8 x i32> [[TMP5]], i32 5
; CHECK-NEXT:    [[TMP17:%.*]] = sext i32 [[TMP16]] to i64
; CHECK-NEXT:    [[T115:%.*]] = getelementptr inbounds i32, ptr [[P_0]], i64 [[TMP17]]
; CHECK-NEXT:    [[L_6:%.*]] = load i32, ptr [[T115]], align 4
; CHECK-NEXT:    [[TMP18:%.*]] = extractelement <8 x i32> [[TMP5]], i32 6
; CHECK-NEXT:    [[TMP19:%.*]] = sext i32 [[TMP18]] to i64
; CHECK-NEXT:    [[T126:%.*]] = getelementptr inbounds i32, ptr [[P_0]], i64 [[TMP19]]
; CHECK-NEXT:    [[L_7:%.*]] = load i32, ptr [[T126]], align 4
; CHECK-NEXT:    [[TMP20:%.*]] = extractelement <8 x i32> [[TMP5]], i32 7
; CHECK-NEXT:    [[TMP21:%.*]] = sext i32 [[TMP20]] to i64
; CHECK-NEXT:    [[T137:%.*]] = getelementptr inbounds i32, ptr [[P_0]], i64 [[TMP21]]
; CHECK-NEXT:    [[L_8:%.*]] = load i32, ptr [[T137]], align 4
; CHECK-NEXT:    call void @use(i32 [[L_1]], i32 [[L_2]], i32 [[L_3]], i32 [[L_4]], i32 [[L_5]], i32 [[L_6]], i32 [[L_7]], i32 [[L_8]])
; CHECK-NEXT:    ret void
;
  %p.0 = load ptr, ptr @global, align 8

  %idx.0 = zext i32 %idx.i32 to i64
  %idx.1 = add nsw i64 %idx.0, 1
  %idx.2 = add nsw i64 %idx.0, 2
  %idx.3 = add nsw i64 %idx.0, 3
  %idx.4 = add nsw i64 %idx.0, 4
  %idx.5 = add nsw i64 %idx.0, 5
  %idx.6 = add nsw i64 %idx.0, 6
  %idx.7 = add nsw i64 %idx.0, 7

  %t53 = getelementptr inbounds i16, ptr %p.1, i64 %idx.0
  %op1.l = load i16, ptr %t53, align 2
  %op1.ext = zext i16 %op1.l to i64
  %t56 = getelementptr inbounds i16, ptr %p.2, i64 %idx.0
  %op2.l = load i16, ptr %t56, align 2
  %op2.ext = zext i16 %op2.l to i64
  %sub.1 = sub nsw i64 %op1.ext, %op2.ext

  %t60 = getelementptr inbounds i32, ptr %p.0, i64 %sub.1
  %l.1 = load i32, ptr %t60, align 4

  %t64 = getelementptr inbounds i16, ptr %p.1, i64 %idx.1
  %t65 = load i16, ptr %t64, align 2
  %t66 = zext i16 %t65 to i64
  %t67 = getelementptr inbounds i16, ptr %p.2, i64 %idx.1
  %t68 = load i16, ptr %t67, align 2
  %t69 = zext i16 %t68 to i64
  %sub.2 = sub nsw i64 %t66, %t69

  %t71 = getelementptr inbounds i32, ptr %p.0, i64 %sub.2
  %l.2 = load i32, ptr %t71, align 4

  %t75 = getelementptr inbounds i16, ptr %p.1, i64 %idx.2
  %t76 = load i16, ptr %t75, align 2
  %t77 = zext i16 %t76 to i64
  %t78 = getelementptr inbounds i16, ptr %p.2, i64 %idx.2
  %t79 = load i16, ptr %t78, align 2
  %t80 = zext i16 %t79 to i64
  %sub.3 = sub nsw i64 %t77, %t80

  %t82 = getelementptr inbounds i32, ptr %p.0, i64 %sub.3
  %l.3 = load i32, ptr %t82, align 4

  %t86 = getelementptr inbounds i16, ptr %p.1, i64 %idx.3
  %t87 = load i16, ptr %t86, align 2
  %t88 = zext i16 %t87 to i64
  %t89 = getelementptr inbounds i16, ptr %p.2, i64 %idx.3
  %t90 = load i16, ptr %t89, align 2
  %t91 = zext i16 %t90 to i64
  %sub.4 = sub nsw i64 %t88, %t91

  %t93 = getelementptr inbounds i32, ptr %p.0, i64 %sub.4
  %l.4 = load i32, ptr %t93, align 4

  %t97 = getelementptr inbounds i16, ptr %p.1, i64 %idx.4
  %t98 = load i16, ptr %t97, align 2
  %t99 = zext i16 %t98 to i64
  %t100 = getelementptr inbounds i16, ptr %p.2, i64 %idx.4
  %t101 = load i16, ptr %t100, align 2
  %t102 = zext i16 %t101 to i64
  %sub.5 = sub nsw i64 %t99, %t102

  %t104 = getelementptr inbounds i32, ptr %p.0, i64 %sub.5
  %l.5 = load i32, ptr %t104, align 4

  %t108 = getelementptr inbounds i16, ptr %p.1, i64 %idx.5
  %t109 = load i16, ptr %t108, align 2
  %t110 = zext i16 %t109 to i64
  %t111 = getelementptr inbounds i16, ptr %p.2, i64 %idx.5
  %t112 = load i16, ptr %t111, align 2
  %t113 = zext i16 %t112 to i64
  %sub.6 = sub nsw i64 %t110, %t113

  %t115 = getelementptr inbounds i32, ptr %p.0, i64 %sub.6
  %l.6 = load i32, ptr %t115, align 4

  %t119 = getelementptr inbounds i16, ptr %p.1, i64 %idx.6
  %t120 = load i16, ptr %t119, align 2
  %t121 = zext i16 %t120 to i64
  %t122 = getelementptr inbounds i16, ptr %p.2, i64 %idx.6
  %t123 = load i16, ptr %t122, align 2
  %t124 = zext i16 %t123 to i64
  %sub.7 = sub nsw i64 %t121, %t124

  %t126 = getelementptr inbounds i32, ptr %p.0, i64 %sub.7
  %l.7 = load i32, ptr %t126, align 4

  %t130 = getelementptr inbounds i16, ptr %p.1, i64 %idx.7
  %t131 = load i16, ptr %t130, align 2
  %t132 = zext i16 %t131 to i64
  %t133 = getelementptr inbounds i16, ptr %p.2, i64 %idx.7
  %t134 = load i16, ptr %t133, align 2
  %t135 = zext i16 %t134 to i64
  %sub.8 = sub nsw i64 %t132, %t135

  %t137 = getelementptr inbounds i32, ptr %p.0, i64 %sub.8
  %l.8 = load i32, ptr %t137, align 4

  call void @use(i32 %l.1, i32 %l.2, i32 %l.3, i32 %l.4, i32 %l.5, i32 %l.6, i32 %l.7, i32 %l.8)
  ret void
}

declare void @use(i32, i32, i32, i32, i32, i32, i32, i32)

; RUN: llc -mtriple arm-unknown -global-isel -stop-after=irtranslator %s -o - | FileCheck %s

define void @test_void_return() {
; CHECK-LABEL: name: test_void_return
; CHECK: BX_RET 14, _
entry:
  ret void
}

define i8 @test_add_i8(i8 %x, i8 %y) {
; CHECK-LABEL: name: test_add_i8
; CHECK: liveins: %r0, %r1
; CHECK-DAG: [[VREGX:%[0-9]+]](s8) = COPY %r0
; CHECK-DAG: [[VREGY:%[0-9]+]](s8) = COPY %r1
; CHECK: [[SUM:%[0-9]+]](s8) = G_ADD [[VREGX]], [[VREGY]]
; CHECK: %r0 = COPY [[SUM]](s8)
; CHECK: BX_RET 14, _, implicit %r0
entry:
  %sum = add i8 %x, %y
  ret i8 %sum
}

define i16 @test_add_i16(i16 %x, i16 %y) {
; CHECK-LABEL: name: test_add_i16
; CHECK: liveins: %r0, %r1
; CHECK-DAG: [[VREGX:%[0-9]+]](s16) = COPY %r0
; CHECK-DAG: [[VREGY:%[0-9]+]](s16) = COPY %r1
; CHECK: [[SUM:%[0-9]+]](s16) = G_ADD [[VREGX]], [[VREGY]]
; CHECK: %r0 = COPY [[SUM]](s16)
; CHECK: BX_RET 14, _, implicit %r0
entry:
  %sum = add i16 %x, %y
  ret i16 %sum
}

define i32 @test_add_i32(i32 %x, i32 %y) {
; CHECK-LABEL: name: test_add_i32
; CHECK: liveins: %r0, %r1
; CHECK-DAG: [[VREGX:%[0-9]+]](s32) = COPY %r0
; CHECK-DAG: [[VREGY:%[0-9]+]](s32) = COPY %r1
; CHECK: [[SUM:%[0-9]+]](s32) = G_ADD [[VREGX]], [[VREGY]]
; CHECK: %r0 = COPY [[SUM]](s32)
; CHECK: BX_RET 14, _, implicit %r0
entry:
  %sum = add i32 %x, %y
  ret i32 %sum
}

define i32 @test_many_args(i32 %p0, i32 %p1, i32 %p2, i32 %p3, i32 %p4, i32 %p5) {
; CHECK-LABEL: name: test_many_args
; CHECK: fixedStack:
; CHECK-DAG: id: [[P4:[0-9]]]{{.*}}offset: 0{{.*}}size: 4
; CHECK-DAG: id: [[P5:[0-9]]]{{.*}}offset: 4{{.*}}size: 4
; CHECK: liveins: %r0, %r1, %r2, %r3
; CHECK: [[VREGP2:%[0-9]+]]{{.*}} = COPY %r2
; CHECK: [[FIP5:%[0-9]+]]{{.*}} = G_FRAME_INDEX %fixed-stack.[[P5]]
; CHECK: [[VREGP5:%[0-9]+]]{{.*}} = G_LOAD [[FIP5]]
; CHECK: [[SUM:%[0-9]+]]{{.*}} = G_ADD [[VREGP2]], [[VREGP5]]
; CHECK: %r0 = COPY [[SUM]]
; CHECK: BX_RET 14, _, implicit %r0
entry:
  %sum = add i32 %p2, %p5
  ret i32 %sum
}

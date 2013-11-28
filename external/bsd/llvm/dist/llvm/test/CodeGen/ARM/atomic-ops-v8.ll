; RUN: llc -mtriple=armv8-none-linux-gnu -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=thumbv8-none-linux-gnu -verify-machineinstrs < %s | FileCheck %s

@var8 = global i8 0
@var16 = global i16 0
@var32 = global i32 0
@var64 = global i64 0

define i8 @test_atomic_load_add_i8(i8 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_add_i8:
   %old = atomicrmw add i8* @var8, i8 %offset seq_cst
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var8
; CHECK: movt r[[ADDR]], :upper16:var8

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaexb r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: add{{s?}} [[NEW:r[0-9]+]], r[[OLD]], r0
; CHECK-NEXT: stlexb [[STATUS:r[0-9]+]], [[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i8 %old
}

define i16 @test_atomic_load_add_i16(i16 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_add_i16:
   %old = atomicrmw add i16* @var16, i16 %offset acquire
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var16
; CHECK: movt r[[ADDR]], :upper16:var16

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaexh r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: add{{s?}} [[NEW:r[0-9]+]], r[[OLD]], r0
; CHECK-NEXT: strexh [[STATUS:r[0-9]+]], [[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i16 %old
}

define i32 @test_atomic_load_add_i32(i32 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_add_i32:
   %old = atomicrmw add i32* @var32, i32 %offset release
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var32
; CHECK: movt r[[ADDR]], :upper16:var32

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldrex r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: add{{s?}} [[NEW:r[0-9]+]], r[[OLD]], r0
; CHECK-NEXT: stlex [[STATUS:r[0-9]+]], [[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i32 %old
}

define i64 @test_atomic_load_add_i64(i64 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_add_i64:
   %old = atomicrmw add i64* @var64, i64 %offset monotonic
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var64
; CHECK: movt r[[ADDR]], :upper16:var64

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldrexd r[[OLD1:[0-9]+]], r[[OLD2:[0-9]+]], [r[[ADDR]]]
  ; r0, r1 below is a reasonable guess but could change: it certainly comes into the
  ; function there.
; CHECK-NEXT: adds [[NEW1:r[0-9]+]], r[[OLD1]], r0
; CHECK-NEXT: adc{{(\.w)?}}  [[NEW2:r[0-9]+]], r[[OLD2]], r1
; CHECK-NEXT: strexd [[STATUS:r[0-9]+]], [[NEW1]], [[NEW2]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD1]]
; CHECK-NEXT: mov r1, r[[OLD2]]
   ret i64 %old
}

define i8 @test_atomic_load_sub_i8(i8 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_sub_i8:
   %old = atomicrmw sub i8* @var8, i8 %offset monotonic
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var8
; CHECK: movt r[[ADDR]], :upper16:var8

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldrexb r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: sub{{s?}} [[NEW:r[0-9]+]], r[[OLD]], r0
; CHECK-NEXT: strexb [[STATUS:r[0-9]+]], [[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i8 %old
}

define i16 @test_atomic_load_sub_i16(i16 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_sub_i16:
   %old = atomicrmw sub i16* @var16, i16 %offset release
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var16
; CHECK: movt r[[ADDR]], :upper16:var16

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldrexh r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: sub{{s?}} [[NEW:r[0-9]+]], r[[OLD]], r0
; CHECK-NEXT: stlexh [[STATUS:r[0-9]+]], [[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i16 %old
}

define i32 @test_atomic_load_sub_i32(i32 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_sub_i32:
   %old = atomicrmw sub i32* @var32, i32 %offset acquire
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var32
; CHECK: movt r[[ADDR]], :upper16:var32

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaex r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: sub{{s?}} [[NEW:r[0-9]+]], r[[OLD]], r0
; CHECK-NEXT: strex [[STATUS:r[0-9]+]], [[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i32 %old
}

define i64 @test_atomic_load_sub_i64(i64 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_sub_i64:
   %old = atomicrmw sub i64* @var64, i64 %offset seq_cst
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var64
; CHECK: movt r[[ADDR]], :upper16:var64

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaexd r[[OLD1:[0-9]+]], r[[OLD2:[0-9]+]], [r[[ADDR]]]
  ; r0, r1 below is a reasonable guess but could change: it certainly comes into the
  ; function there.
; CHECK-NEXT: subs [[NEW1:r[0-9]+]], r[[OLD1]], r0
; CHECK-NEXT: sbc{{(\.w)?}}  [[NEW2:r[0-9]+]], r[[OLD2]], r1
; CHECK-NEXT: stlexd [[STATUS:r[0-9]+]], [[NEW1]], [[NEW2]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD1]]
; CHECK-NEXT: mov r1, r[[OLD2]]
   ret i64 %old
}

define i8 @test_atomic_load_and_i8(i8 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_and_i8:
   %old = atomicrmw and i8* @var8, i8 %offset release
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var8
; CHECK: movt r[[ADDR]], :upper16:var8

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldrexb r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: and{{(\.w)?}} [[NEW:r[0-9]+]], r[[OLD]], r0
; CHECK-NEXT: stlexb [[STATUS:r[0-9]+]], [[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i8 %old
}

define i16 @test_atomic_load_and_i16(i16 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_and_i16:
   %old = atomicrmw and i16* @var16, i16 %offset monotonic
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var16
; CHECK: movt r[[ADDR]], :upper16:var16

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldrexh r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: and{{(\.w)?}} [[NEW:r[0-9]+]], r[[OLD]], r0
; CHECK-NEXT: strexh [[STATUS:r[0-9]+]], [[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i16 %old
}

define i32 @test_atomic_load_and_i32(i32 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_and_i32:
   %old = atomicrmw and i32* @var32, i32 %offset seq_cst
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var32
; CHECK: movt r[[ADDR]], :upper16:var32

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaex r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: and{{(\.w)?}} [[NEW:r[0-9]+]], r[[OLD]], r0
; CHECK-NEXT: stlex [[STATUS:r[0-9]+]], [[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i32 %old
}

define i64 @test_atomic_load_and_i64(i64 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_and_i64:
   %old = atomicrmw and i64* @var64, i64 %offset acquire
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var64
; CHECK: movt r[[ADDR]], :upper16:var64

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaexd r[[OLD1:[0-9]+]], r[[OLD2:[0-9]+]], [r[[ADDR]]]
  ; r0, r1 below is a reasonable guess but could change: it certainly comes into the
  ; function there.
; CHECK-NEXT: and{{(\.w)?}} [[NEW1:r[0-9]+]], r[[OLD1]], r0
; CHECK-NEXT: and{{(\.w)?}} [[NEW2:r[0-9]+]], r[[OLD2]], r1
; CHECK-NEXT: strexd [[STATUS:r[0-9]+]], [[NEW1]], [[NEW2]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD1]]
; CHECK-NEXT: mov r1, r[[OLD2]]
   ret i64 %old
}

define i8 @test_atomic_load_or_i8(i8 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_or_i8:
   %old = atomicrmw or i8* @var8, i8 %offset seq_cst
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var8
; CHECK: movt r[[ADDR]], :upper16:var8

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaexb r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: orr{{(\.w)?}} [[NEW:r[0-9]+]], r[[OLD]], r0
; CHECK-NEXT: stlexb [[STATUS:r[0-9]+]], [[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i8 %old
}

define i16 @test_atomic_load_or_i16(i16 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_or_i16:
   %old = atomicrmw or i16* @var16, i16 %offset monotonic
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var16
; CHECK: movt r[[ADDR]], :upper16:var16

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldrexh r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: orr{{(\.w)?}} [[NEW:r[0-9]+]], r[[OLD]], r0
; CHECK-NEXT: strexh [[STATUS:r[0-9]+]], [[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i16 %old
}

define i32 @test_atomic_load_or_i32(i32 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_or_i32:
   %old = atomicrmw or i32* @var32, i32 %offset acquire
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var32
; CHECK: movt r[[ADDR]], :upper16:var32

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaex r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: orr{{(\.w)?}} [[NEW:r[0-9]+]], r[[OLD]], r0
; CHECK-NEXT: strex [[STATUS:r[0-9]+]], [[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i32 %old
}

define i64 @test_atomic_load_or_i64(i64 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_or_i64:
   %old = atomicrmw or i64* @var64, i64 %offset release
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var64
; CHECK: movt r[[ADDR]], :upper16:var64

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldrexd r[[OLD1:[0-9]+]], r[[OLD2:[0-9]+]], [r[[ADDR]]]
  ; r0, r1 below is a reasonable guess but could change: it certainly comes into the
  ; function there.
; CHECK-NEXT: orr{{(\.w)?}} [[NEW1:r[0-9]+]], r[[OLD1]], r0
; CHECK-NEXT: orr{{(\.w)?}} [[NEW2:r[0-9]+]], r[[OLD2]], r1
; CHECK-NEXT: stlexd [[STATUS:r[0-9]+]], [[NEW1]], [[NEW2]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD1]]
; CHECK-NEXT: mov r1, r[[OLD2]]
   ret i64 %old
}

define i8 @test_atomic_load_xor_i8(i8 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_xor_i8:
   %old = atomicrmw xor i8* @var8, i8 %offset acquire
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var8
; CHECK: movt r[[ADDR]], :upper16:var8

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaexb r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: eor{{(\.w)?}} [[NEW:r[0-9]+]], r[[OLD]], r0
; CHECK-NEXT: strexb [[STATUS:r[0-9]+]], [[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i8 %old
}

define i16 @test_atomic_load_xor_i16(i16 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_xor_i16:
   %old = atomicrmw xor i16* @var16, i16 %offset release
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var16
; CHECK: movt r[[ADDR]], :upper16:var16

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldrexh r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: eor{{(\.w)?}} [[NEW:r[0-9]+]], r[[OLD]], r0
; CHECK-NEXT: stlexh [[STATUS:r[0-9]+]], [[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i16 %old
}

define i32 @test_atomic_load_xor_i32(i32 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_xor_i32:
   %old = atomicrmw xor i32* @var32, i32 %offset seq_cst
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var32
; CHECK: movt r[[ADDR]], :upper16:var32

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaex r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: eor{{(\.w)?}} [[NEW:r[0-9]+]], r[[OLD]], r0
; CHECK-NEXT: stlex [[STATUS:r[0-9]+]], [[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i32 %old
}

define i64 @test_atomic_load_xor_i64(i64 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_xor_i64:
   %old = atomicrmw xor i64* @var64, i64 %offset monotonic
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var64
; CHECK: movt r[[ADDR]], :upper16:var64

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldrexd r[[OLD1:[0-9]+]], r[[OLD2:[0-9]+]], [r[[ADDR]]]
  ; r0, r1 below is a reasonable guess but could change: it certainly comes into the
  ; function there.
; CHECK-NEXT: eor{{(\.w)?}} [[NEW1:r[0-9]+]], r[[OLD1]], r0
; CHECK-NEXT: eor{{(\.w)?}} [[NEW2:r[0-9]+]], r[[OLD2]], r1
; CHECK-NEXT: strexd [[STATUS:r[0-9]+]], [[NEW1]], [[NEW2]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD1]]
; CHECK-NEXT: mov r1, r[[OLD2]]
   ret i64 %old
}

define i8 @test_atomic_load_xchg_i8(i8 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_xchg_i8:
   %old = atomicrmw xchg i8* @var8, i8 %offset monotonic
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var8
; CHECK: movt r[[ADDR]], :upper16:var8

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldrexb r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: strexb [[STATUS:r[0-9]+]], r0, [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i8 %old
}

define i16 @test_atomic_load_xchg_i16(i16 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_xchg_i16:
   %old = atomicrmw xchg i16* @var16, i16 %offset seq_cst
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var16
; CHECK: movt r[[ADDR]], :upper16:var16

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaexh r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: stlexh [[STATUS:r[0-9]+]], r0, [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i16 %old
}

define i32 @test_atomic_load_xchg_i32(i32 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_xchg_i32:
   %old = atomicrmw xchg i32* @var32, i32 %offset release
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var32
; CHECK: movt r[[ADDR]], :upper16:var32

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldrex r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: stlex [[STATUS:r[0-9]+]], r0, [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i32 %old
}

define i64 @test_atomic_load_xchg_i64(i64 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_xchg_i64:
   %old = atomicrmw xchg i64* @var64, i64 %offset acquire
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var64
; CHECK: movt r[[ADDR]], :upper16:var64

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaexd r[[OLD1:[0-9]+]], r[[OLD2:[0-9]+]], [r[[ADDR]]]
  ; r0, r1 below is a reasonable guess but could change: it certainly comes into the
  ; function there.
; CHECK-NEXT: strexd [[STATUS:r[0-9]+]], r0, r1, [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD1]]
; CHECK-NEXT: mov r1, r[[OLD2]]
   ret i64 %old
}

define i8 @test_atomic_load_min_i8(i8 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_min_i8:
   %old = atomicrmw min i8* @var8, i8 %offset acquire
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var8
; CHECK: movt r[[ADDR]], :upper16:var8

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaexb r[[OLD:[0-9]+]], [r[[ADDR]]]
; CHECK-NEXT: sxtb r[[OLDX:[0-9]+]], r[[OLD]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: cmp r[[OLDX]], r0
; Thumb mode: it ge
; CHECK:      movge r[[OLDX]], r0
; CHECK-NEXT: strexb [[STATUS:r[0-9]+]], r[[OLDX]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i8 %old
}

define i16 @test_atomic_load_min_i16(i16 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_min_i16:
   %old = atomicrmw min i16* @var16, i16 %offset release
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var16
; CHECK: movt r[[ADDR]], :upper16:var16

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldrexh r[[OLD:[0-9]+]], [r[[ADDR]]]
; CHECK-NEXT: sxth r[[OLDX:[0-9]+]], r[[OLD]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: cmp r[[OLDX]], r0
; Thumb mode: it ge
; CHECK:      movge r[[OLDX]], r0
; CHECK-NEXT: stlexh [[STATUS:r[0-9]+]], r[[OLDX]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i16 %old
}

define i32 @test_atomic_load_min_i32(i32 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_min_i32:
   %old = atomicrmw min i32* @var32, i32 %offset monotonic
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var32
; CHECK: movt r[[ADDR]], :upper16:var32

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldrex r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: mov r[[NEW:[0-9]+]], r0
; CHECK-NEXT: cmp r[[OLD]], r0
; Thumb mode: it lt
; CHECK:      movlt r[[NEW]], r[[OLD]]
; CHECK-NEXT: strex [[STATUS:r[0-9]+]], r[[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i32 %old
}

define i64 @test_atomic_load_min_i64(i64 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_min_i64:
   %old = atomicrmw min i64* @var64, i64 %offset seq_cst
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var64
; CHECK: movt r[[ADDR]], :upper16:var64

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaexd r[[OLD1:[0-9]+]], r[[OLD2:[0-9]+]], [r[[ADDR]]]
  ; r0, r1 below is a reasonable guess but could change: it certainly comes into the
  ; function there.
; CHECK-NEXT: subs [[NEW:r[0-9]+]], r[[OLD1]], r0
; CHECK-NEXT: sbcs{{(\.w)?}} [[NEW]], r[[OLD2]], r1
; CHECK-NEXT: blt .LBB{{[0-9]+}}_3
; CHECK-NEXT: BB#2:
; CHECK-NEXT: stlexd [[STATUS:r[0-9]+]], r0, r1, [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD1]]
; CHECK-NEXT: mov r1, r[[OLD2]]
   ret i64 %old
}

define i8 @test_atomic_load_max_i8(i8 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_max_i8:
   %old = atomicrmw max i8* @var8, i8 %offset seq_cst
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var8
; CHECK: movt r[[ADDR]], :upper16:var8

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaexb r[[OLD:[0-9]+]], [r[[ADDR]]]
; CHECK-NEXT: sxtb r[[OLDX:[0-9]+]], r[[OLD]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: cmp r[[OLDX]], r0
; Thumb mode: it le
; CHECK:      movle r[[OLDX]], r0
; CHECK-NEXT: stlexb [[STATUS:r[0-9]+]], r[[OLDX]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i8 %old
}

define i16 @test_atomic_load_max_i16(i16 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_max_i16:
   %old = atomicrmw max i16* @var16, i16 %offset acquire
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var16
; CHECK: movt r[[ADDR]], :upper16:var16

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaexh r[[OLD:[0-9]+]], [r[[ADDR]]]
; CHECK-NEXT: sxth r[[OLDX:[0-9]+]], r[[OLD]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: cmp r[[OLDX]], r0
; Thumb mode: it le
; CHECK:      movle r[[OLDX]], r0
; CHECK-NEXT: strexh [[STATUS:r[0-9]+]], r[[OLDX]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i16 %old
}

define i32 @test_atomic_load_max_i32(i32 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_max_i32:
   %old = atomicrmw max i32* @var32, i32 %offset release
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var32
; CHECK: movt r[[ADDR]], :upper16:var32

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldrex r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: mov r[[NEW:[0-9]+]], r0
; CHECK-NEXT: cmp r[[OLD]], r0
; Thumb mode: it gt
; CHECK:      movgt r[[NEW]], r[[OLD]]
; CHECK-NEXT: stlex [[STATUS:r[0-9]+]], r[[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i32 %old
}

define i64 @test_atomic_load_max_i64(i64 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_max_i64:
   %old = atomicrmw max i64* @var64, i64 %offset monotonic
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var64
; CHECK: movt r[[ADDR]], :upper16:var64

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldrexd r[[OLD1:[0-9]+]], r[[OLD2:[0-9]+]], [r[[ADDR]]]
  ; r0, r1 below is a reasonable guess but could change: it certainly comes into the
  ; function there.
; CHECK-NEXT: subs [[NEW:r[0-9]+]], r[[OLD1]], r0
; CHECK-NEXT: sbcs{{(\.w)?}} [[NEW]], r[[OLD2]], r1
; CHECK-NEXT: bge .LBB{{[0-9]+}}_3
; CHECK-NEXT: BB#2:
; CHECK-NEXT: strexd [[STATUS:r[0-9]+]], r0, r1, [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD1]]
; CHECK-NEXT: mov r1, r[[OLD2]]
   ret i64 %old
}

define i8 @test_atomic_load_umin_i8(i8 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_umin_i8:
   %old = atomicrmw umin i8* @var8, i8 %offset monotonic
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var8
; CHECK: movt r[[ADDR]], :upper16:var8

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldrexb r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: mov r[[NEW:[0-9]+]], r0
; CHECK-NEXT: cmp r[[OLD]], r0
; Thumb mode: it lo
; CHECK:      movlo r[[NEW]], r[[OLD]]
; CHECK-NEXT: strexb [[STATUS:r[0-9]+]], r[[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i8 %old
}

define i16 @test_atomic_load_umin_i16(i16 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_umin_i16:
   %old = atomicrmw umin i16* @var16, i16 %offset acquire
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var16
; CHECK: movt r[[ADDR]], :upper16:var16

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaexh r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: mov r[[NEW:[0-9]+]], r0
; CHECK-NEXT: cmp r[[OLD]], r0
; Thumb mode: it lo
; CHECK:      movlo r[[NEW]], r[[OLD]]
; CHECK-NEXT: strexh [[STATUS:r[0-9]+]], r[[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i16 %old
}

define i32 @test_atomic_load_umin_i32(i32 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_umin_i32:
   %old = atomicrmw umin i32* @var32, i32 %offset seq_cst
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var32
; CHECK: movt r[[ADDR]], :upper16:var32

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaex r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: mov r[[NEW:[0-9]+]], r0
; CHECK-NEXT: cmp r[[OLD]], r0
; Thumb mode: it lo
; CHECK:      movlo r[[NEW]], r[[OLD]]
; CHECK-NEXT: stlex [[STATUS:r[0-9]+]], r[[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i32 %old
}

define i64 @test_atomic_load_umin_i64(i64 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_umin_i64:
   %old = atomicrmw umin i64* @var64, i64 %offset acq_rel
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var64
; CHECK: movt r[[ADDR]], :upper16:var64

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaexd r[[OLD1:[0-9]+]], r[[OLD2:[0-9]+]], [r[[ADDR]]]
  ; r0, r1 below is a reasonable guess but could change: it certainly comes into the
  ; function there.
; CHECK-NEXT: subs [[NEW:r[0-9]+]], r[[OLD1]], r0
; CHECK-NEXT: sbcs{{(\.w)?}} [[NEW]], r[[OLD2]], r1
; CHECK-NEXT: blo .LBB{{[0-9]+}}_3
; CHECK-NEXT: BB#2:
; CHECK-NEXT: stlexd [[STATUS:r[0-9]+]], r0, r1, [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD1]]
; CHECK-NEXT: mov r1, r[[OLD2]]
   ret i64 %old
}

define i8 @test_atomic_load_umax_i8(i8 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_umax_i8:
   %old = atomicrmw umax i8* @var8, i8 %offset acq_rel
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var8
; CHECK: movt r[[ADDR]], :upper16:var8

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaexb r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: mov r[[NEW:[0-9]+]], r0
; CHECK-NEXT: cmp r[[OLD]], r0
; Thumb mode: it hi
; CHECK:      movhi r[[NEW]], r[[OLD]]
; CHECK-NEXT: stlexb [[STATUS:r[0-9]+]], r[[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i8 %old
}

define i16 @test_atomic_load_umax_i16(i16 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_umax_i16:
   %old = atomicrmw umax i16* @var16, i16 %offset monotonic
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var16
; CHECK: movt r[[ADDR]], :upper16:var16

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldrexh r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: mov r[[NEW:[0-9]+]], r0
; CHECK-NEXT: cmp r[[OLD]], r0
; Thumb mode: it hi
; CHECK:      movhi r[[NEW]], r[[OLD]]
; CHECK-NEXT: strexh [[STATUS:r[0-9]+]], r[[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i16 %old
}

define i32 @test_atomic_load_umax_i32(i32 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_umax_i32:
   %old = atomicrmw umax i32* @var32, i32 %offset seq_cst
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var32
; CHECK: movt r[[ADDR]], :upper16:var32

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaex r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: mov r[[NEW:[0-9]+]], r0
; CHECK-NEXT: cmp r[[OLD]], r0
; Thumb mode: it hi
; CHECK:      movhi r[[NEW]], r[[OLD]]
; CHECK-NEXT: stlex [[STATUS:r[0-9]+]], r[[NEW]], [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i32 %old
}

define i64 @test_atomic_load_umax_i64(i64 %offset) nounwind {
; CHECK-LABEL: test_atomic_load_umax_i64:
   %old = atomicrmw umax i64* @var64, i64 %offset release
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var64
; CHECK: movt r[[ADDR]], :upper16:var64

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldrexd r[[OLD1:[0-9]+]], r[[OLD2:[0-9]+]], [r[[ADDR]]]
  ; r0, r1 below is a reasonable guess but could change: it certainly comes into the
  ; function there.
; CHECK-NEXT: subs [[NEW:r[0-9]+]], r[[OLD1]], r0
; CHECK-NEXT: sbcs{{(\.w)?}} [[NEW]], r[[OLD2]], r1
; CHECK-NEXT: bhs .LBB{{[0-9]+}}_3
; CHECK-NEXT: BB#2:
; CHECK-NEXT: stlexd [[STATUS:r[0-9]+]], r0, r1, [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD1]]
; CHECK-NEXT: mov r1, r[[OLD2]]
   ret i64 %old
}

define i8 @test_atomic_cmpxchg_i8(i8 %wanted, i8 %new) nounwind {
; CHECK-LABEL: test_atomic_cmpxchg_i8:
   %old = cmpxchg i8* @var8, i8 %wanted, i8 %new acquire
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var8
; CHECK: movt r[[ADDR]], :upper16:var8

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaexb r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: cmp r[[OLD]], r0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_3
; CHECK-NEXT: BB#2:
  ; As above, r1 is a reasonable guess.
; CHECK-NEXT: strexb [[STATUS:r[0-9]+]], r1, [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i8 %old
}

define i16 @test_atomic_cmpxchg_i16(i16 %wanted, i16 %new) nounwind {
; CHECK-LABEL: test_atomic_cmpxchg_i16:
   %old = cmpxchg i16* @var16, i16 %wanted, i16 %new seq_cst
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var16
; CHECK: movt r[[ADDR]], :upper16:var16

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldaexh r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: cmp r[[OLD]], r0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_3
; CHECK-NEXT: BB#2:
  ; As above, r1 is a reasonable guess.
; CHECK-NEXT: stlexh [[STATUS:r[0-9]+]], r1, [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i16 %old
}

define i32 @test_atomic_cmpxchg_i32(i32 %wanted, i32 %new) nounwind {
; CHECK-LABEL: test_atomic_cmpxchg_i32:
   %old = cmpxchg i32* @var32, i32 %wanted, i32 %new release
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var32
; CHECK: movt r[[ADDR]], :upper16:var32

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldrex r[[OLD:[0-9]+]], [r[[ADDR]]]
  ; r0 below is a reasonable guess but could change: it certainly comes into the
  ;  function there.
; CHECK-NEXT: cmp r[[OLD]], r0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_3
; CHECK-NEXT: BB#2:
  ; As above, r1 is a reasonable guess.
; CHECK-NEXT: stlex [[STATUS:r[0-9]+]], r1, [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, r[[OLD]]
   ret i32 %old
}

define i64 @test_atomic_cmpxchg_i64(i64 %wanted, i64 %new) nounwind {
; CHECK-LABEL: test_atomic_cmpxchg_i64:
   %old = cmpxchg i64* @var64, i64 %wanted, i64 %new monotonic
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var64
; CHECK: movt r[[ADDR]], :upper16:var64

; CHECK: .LBB{{[0-9]+}}_1:
; CHECK-NEXT: ldrexd [[OLD1:r[0-9]+|lr]], [[OLD2:r[0-9]+|lr]], [r[[ADDR]]]
  ; r0, r1 below is a reasonable guess but could change: it certainly comes into the
  ; function there.
; CHECK-NEXT: cmp   [[OLD1]], r0
; Thumb mode: it eq
; CHECK:      cmpeq [[OLD2]], r1
; CHECK-NEXT: bne .LBB{{[0-9]+}}_3
; CHECK-NEXT: BB#2:
  ; As above, r2, r3 is a reasonable guess.
; CHECK-NEXT: strexd [[STATUS:r[0-9]+]], r2, r3, [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

; CHECK: mov r0, [[OLD1]]
; CHECK-NEXT: mov r1, [[OLD2]]
   ret i64 %old
}

define i8 @test_atomic_load_monotonic_i8() nounwind {
; CHECK-LABEL: test_atomic_load_monotonic_i8:
  %val = load atomic i8* @var8 monotonic, align 1
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var8
; CHECK: movt r[[ADDR]], :upper16:var8
; CHECK: ldrb r0, [r[[ADDR]]]
; CHECK-NOT: dmb
; CHECK-NOT: mcr

  ret i8 %val
}

define i8 @test_atomic_load_monotonic_regoff_i8(i64 %base, i64 %off) nounwind {
; CHECK-LABEL: test_atomic_load_monotonic_regoff_i8:
  %addr_int = add i64 %base, %off
  %addr = inttoptr i64 %addr_int to i8*

  %val = load atomic i8* %addr monotonic, align 1
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: ldrb r0, [r0, r2]
; CHECK-NOT: dmb
; CHECK-NOT: mcr

  ret i8 %val
}

define i8 @test_atomic_load_acquire_i8() nounwind {
; CHECK-LABEL: test_atomic_load_acquire_i8:
  %val = load atomic i8* @var8 acquire, align 1
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var8
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movt r[[ADDR]], :upper16:var8
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: ldab r0, [r[[ADDR]]]
; CHECK-NOT: dmb
; CHECK-NOT: mcr
  ret i8 %val
}

define i8 @test_atomic_load_seq_cst_i8() nounwind {
; CHECK-LABEL: test_atomic_load_seq_cst_i8:
  %val = load atomic i8* @var8 seq_cst, align 1
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var8
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movt r[[ADDR]], :upper16:var8
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: ldab r0, [r[[ADDR]]]
; CHECK-NOT: dmb
; CHECK-NOT: mcr
  ret i8 %val
}

define i16 @test_atomic_load_monotonic_i16() nounwind {
; CHECK-LABEL: test_atomic_load_monotonic_i16:
  %val = load atomic i16* @var16 monotonic, align 2
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var16
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movt r[[ADDR]], :upper16:var16
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: ldrh r0, [r[[ADDR]]]
; CHECK-NOT: dmb
; CHECK-NOT: mcr

  ret i16 %val
}

define i32 @test_atomic_load_monotonic_regoff_i32(i64 %base, i64 %off) nounwind {
; CHECK-LABEL: test_atomic_load_monotonic_regoff_i32:
  %addr_int = add i64 %base, %off
  %addr = inttoptr i64 %addr_int to i32*

  %val = load atomic i32* %addr monotonic, align 4
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: ldr r0, [r0, r2]
; CHECK-NOT: dmb
; CHECK-NOT: mcr

  ret i32 %val
}

define i64 @test_atomic_load_seq_cst_i64() nounwind {
; CHECK-LABEL: test_atomic_load_seq_cst_i64:
  %val = load atomic i64* @var64 seq_cst, align 8
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var64
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movt r[[ADDR]], :upper16:var64
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: ldaexd r0, r1, [r[[ADDR]]]
; CHECK-NOT: dmb
; CHECK-NOT: mcr
  ret i64 %val
}

define void @test_atomic_store_monotonic_i8(i8 %val) nounwind {
; CHECK-LABEL: test_atomic_store_monotonic_i8:
  store atomic i8 %val, i8* @var8 monotonic, align 1
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var8
; CHECK: movt r[[ADDR]], :upper16:var8
; CHECK: strb r0, [r[[ADDR]]]

  ret void
}

define void @test_atomic_store_monotonic_regoff_i8(i64 %base, i64 %off, i8 %val) nounwind {
; CHECK-LABEL: test_atomic_store_monotonic_regoff_i8:

  %addr_int = add i64 %base, %off
  %addr = inttoptr i64 %addr_int to i8*

  store atomic i8 %val, i8* %addr monotonic, align 1
; CHECK: ldrb{{(\.w)?}} [[VAL:r[0-9]+]], [sp]
; CHECK: strb [[VAL]], [r0, r2]

  ret void
}

define void @test_atomic_store_release_i8(i8 %val) nounwind {
; CHECK-LABEL: test_atomic_store_release_i8:
  store atomic i8 %val, i8* @var8 release, align 1
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var8
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movt r[[ADDR]], :upper16:var8
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: stlb r0, [r[[ADDR]]]
; CHECK-NOT: dmb
; CHECK-NOT: mcr
  ret void
}

define void @test_atomic_store_seq_cst_i8(i8 %val) nounwind {
; CHECK-LABEL: test_atomic_store_seq_cst_i8:
  store atomic i8 %val, i8* @var8 seq_cst, align 1
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var8
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movt r[[ADDR]], :upper16:var8
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: stlb r0, [r[[ADDR]]]
; CHECK-NOT: dmb
; CHECK-NOT: mcr
  ret void
}

define void @test_atomic_store_monotonic_i16(i16 %val) nounwind {
; CHECK-LABEL: test_atomic_store_monotonic_i16:
  store atomic i16 %val, i16* @var16 monotonic, align 2
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var16
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movt r[[ADDR]], :upper16:var16
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: strh r0, [r[[ADDR]]]
; CHECK-NOT: dmb
; CHECK-NOT: mcr
  ret void
}

define void @test_atomic_store_monotonic_regoff_i32(i64 %base, i64 %off, i32 %val) nounwind {
; CHECK-LABEL: test_atomic_store_monotonic_regoff_i32:

  %addr_int = add i64 %base, %off
  %addr = inttoptr i64 %addr_int to i32*

  store atomic i32 %val, i32* %addr monotonic, align 4
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: ldr [[VAL:r[0-9]+]], [sp]
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: str [[VAL]], [r0, r2]
; CHECK-NOT: dmb
; CHECK-NOT: mcr

  ret void
}

define void @test_atomic_store_release_i64(i64 %val) nounwind {
; CHECK-LABEL: test_atomic_store_release_i64:
  store atomic i64 %val, i64* @var64 release, align 8
; CHECK-NOT: dmb
; CHECK-NOT: mcr
; CHECK: movw r[[ADDR:[0-9]+]], :lower16:var64
; CHECK: movt r[[ADDR]], :upper16:var64

; CHECK: .LBB{{[0-9]+}}_1:
  ; r0, r1 below is a reasonable guess but could change: it certainly comes into the
  ; function there.
; CHECK: stlexd [[STATUS:r[0-9]+]], r0, r1, [r[[ADDR]]]
; CHECK-NEXT: cmp [[STATUS]], #0
; CHECK-NEXT: bne .LBB{{[0-9]+}}_1
; CHECK-NOT: dmb
; CHECK-NOT: mcr

  ret void
}

define i32 @not.barriers(i32* %var, i1 %cond) {
; CHECK-LABEL: not.barriers:
  br i1 %cond, label %atomic_ver, label %simple_ver
simple_ver:
  %oldval = load i32* %var
  %newval = add nsw i32 %oldval, -1
  store i32 %newval, i32* %var
  br label %somewhere
atomic_ver:
  fence seq_cst
  %val = atomicrmw add i32* %var, i32 -1 monotonic
  fence seq_cst
  br label %somewhere
; CHECK: dmb
; CHECK: ldrex
; CHECK: dmb
  ; The key point here is that the second dmb isn't immediately followed by the
  ; simple_ver basic block, which LLVM attempted to do when DMB had been marked
  ; with isBarrier. For now, look for something that looks like "somewhere".
; CHECK-NEXT: mov
somewhere:
  %combined = phi i32 [ %val, %atomic_ver ], [ %newval, %simple_ver]
  ret i32 %combined
}

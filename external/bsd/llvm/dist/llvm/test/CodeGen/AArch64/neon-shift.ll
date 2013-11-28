; RUN: llc < %s -verify-machineinstrs -mtriple=aarch64-none-linux-gnu -mattr=+neon | FileCheck %s

declare <8 x i8> @llvm.arm.neon.vshiftu.v8i8(<8 x i8>, <8 x i8>)
declare <8 x i8> @llvm.arm.neon.vshifts.v8i8(<8 x i8>, <8 x i8>)

define <8 x i8> @test_uqshl_v8i8(<8 x i8> %lhs, <8 x i8> %rhs) {
; CHECK: test_uqshl_v8i8:
  %tmp1 = call <8 x i8> @llvm.arm.neon.vshiftu.v8i8(<8 x i8> %lhs, <8 x i8> %rhs)
; CHECK: ushl v0.8b, v0.8b, v1.8b
  ret <8 x i8> %tmp1
}

define <8 x i8> @test_sqshl_v8i8(<8 x i8> %lhs, <8 x i8> %rhs) {
; CHECK: test_sqshl_v8i8:
  %tmp1 = call <8 x i8> @llvm.arm.neon.vshifts.v8i8(<8 x i8> %lhs, <8 x i8> %rhs)
; CHECK: sshl v0.8b, v0.8b, v1.8b
  ret <8 x i8> %tmp1
}

declare <16 x i8> @llvm.arm.neon.vshiftu.v16i8(<16 x i8>, <16 x i8>)
declare <16 x i8> @llvm.arm.neon.vshifts.v16i8(<16 x i8>, <16 x i8>)

define <16 x i8> @test_ushl_v16i8(<16 x i8> %lhs, <16 x i8> %rhs) {
; CHECK: test_ushl_v16i8:
  %tmp1 = call <16 x i8> @llvm.arm.neon.vshiftu.v16i8(<16 x i8> %lhs, <16 x i8> %rhs)
; CHECK: ushl v0.16b, v0.16b, v1.16b
  ret <16 x i8> %tmp1
}

define <16 x i8> @test_sshl_v16i8(<16 x i8> %lhs, <16 x i8> %rhs) {
; CHECK: test_sshl_v16i8:
  %tmp1 = call <16 x i8> @llvm.arm.neon.vshifts.v16i8(<16 x i8> %lhs, <16 x i8> %rhs)
; CHECK: sshl v0.16b, v0.16b, v1.16b
  ret <16 x i8> %tmp1
}

declare <4 x i16> @llvm.arm.neon.vshiftu.v4i16(<4 x i16>, <4 x i16>)
declare <4 x i16> @llvm.arm.neon.vshifts.v4i16(<4 x i16>, <4 x i16>)

define <4 x i16> @test_ushl_v4i16(<4 x i16> %lhs, <4 x i16> %rhs) {
; CHECK: test_ushl_v4i16:
  %tmp1 = call <4 x i16> @llvm.arm.neon.vshiftu.v4i16(<4 x i16> %lhs, <4 x i16> %rhs)
; CHECK: ushl v0.4h, v0.4h, v1.4h
  ret <4 x i16> %tmp1
}

define <4 x i16> @test_sshl_v4i16(<4 x i16> %lhs, <4 x i16> %rhs) {
; CHECK: test_sshl_v4i16:
  %tmp1 = call <4 x i16> @llvm.arm.neon.vshifts.v4i16(<4 x i16> %lhs, <4 x i16> %rhs)
; CHECK: sshl v0.4h, v0.4h, v1.4h
  ret <4 x i16> %tmp1
}

declare <8 x i16> @llvm.arm.neon.vshiftu.v8i16(<8 x i16>, <8 x i16>)
declare <8 x i16> @llvm.arm.neon.vshifts.v8i16(<8 x i16>, <8 x i16>)

define <8 x i16> @test_ushl_v8i16(<8 x i16> %lhs, <8 x i16> %rhs) {
; CHECK: test_ushl_v8i16:
  %tmp1 = call <8 x i16> @llvm.arm.neon.vshiftu.v8i16(<8 x i16> %lhs, <8 x i16> %rhs)
; CHECK: ushl v0.8h, v0.8h, v1.8h
  ret <8 x i16> %tmp1
}

define <8 x i16> @test_sshl_v8i16(<8 x i16> %lhs, <8 x i16> %rhs) {
; CHECK: test_sshl_v8i16:
  %tmp1 = call <8 x i16> @llvm.arm.neon.vshifts.v8i16(<8 x i16> %lhs, <8 x i16> %rhs)
; CHECK: sshl v0.8h, v0.8h, v1.8h
  ret <8 x i16> %tmp1
}

declare <2 x i32> @llvm.arm.neon.vshiftu.v2i32(<2 x i32>, <2 x i32>)
declare <2 x i32> @llvm.arm.neon.vshifts.v2i32(<2 x i32>, <2 x i32>)

define <2 x i32> @test_ushl_v2i32(<2 x i32> %lhs, <2 x i32> %rhs) {
; CHECK: test_ushl_v2i32:
  %tmp1 = call <2 x i32> @llvm.arm.neon.vshiftu.v2i32(<2 x i32> %lhs, <2 x i32> %rhs)
; CHECK: ushl v0.2s, v0.2s, v1.2s
  ret <2 x i32> %tmp1
}

define <2 x i32> @test_sshl_v2i32(<2 x i32> %lhs, <2 x i32> %rhs) {
; CHECK: test_sshl_v2i32:
  %tmp1 = call <2 x i32> @llvm.arm.neon.vshifts.v2i32(<2 x i32> %lhs, <2 x i32> %rhs)
; CHECK: sshl v0.2s, v0.2s, v1.2s
  ret <2 x i32> %tmp1
}

declare <4 x i32> @llvm.arm.neon.vshiftu.v4i32(<4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.arm.neon.vshifts.v4i32(<4 x i32>, <4 x i32>)

define <4 x i32> @test_ushl_v4i32(<4 x i32> %lhs, <4 x i32> %rhs) {
; CHECK: test_ushl_v4i32:
  %tmp1 = call <4 x i32> @llvm.arm.neon.vshiftu.v4i32(<4 x i32> %lhs, <4 x i32> %rhs)
; CHECK: ushl v0.4s, v0.4s, v1.4s
  ret <4 x i32> %tmp1
}

define <4 x i32> @test_sshl_v4i32(<4 x i32> %lhs, <4 x i32> %rhs) {
; CHECK: test_sshl_v4i32:
  %tmp1 = call <4 x i32> @llvm.arm.neon.vshifts.v4i32(<4 x i32> %lhs, <4 x i32> %rhs)
; CHECK: sshl v0.4s, v0.4s, v1.4s
  ret <4 x i32> %tmp1
}

declare <2 x i64> @llvm.arm.neon.vshiftu.v2i64(<2 x i64>, <2 x i64>)
declare <2 x i64> @llvm.arm.neon.vshifts.v2i64(<2 x i64>, <2 x i64>)

define <2 x i64> @test_ushl_v2i64(<2 x i64> %lhs, <2 x i64> %rhs) {
; CHECK: test_ushl_v2i64:
  %tmp1 = call <2 x i64> @llvm.arm.neon.vshiftu.v2i64(<2 x i64> %lhs, <2 x i64> %rhs)
; CHECK: ushl v0.2d, v0.2d, v1.2d
  ret <2 x i64> %tmp1
}

define <2 x i64> @test_sshl_v2i64(<2 x i64> %lhs, <2 x i64> %rhs) {
; CHECK: test_sshl_v2i64:
  %tmp1 = call <2 x i64> @llvm.arm.neon.vshifts.v2i64(<2 x i64> %lhs, <2 x i64> %rhs)
; CHECK: sshl v0.2d, v0.2d, v1.2d
  ret <2 x i64> %tmp1
}


define <8 x i8> @test_shl_v8i8(<8 x i8> %a) {
; CHECK: test_shl_v8i8:
; CHECK: shl {{v[0-9]+}}.8b, {{v[0-9]+}}.8b, #3
  %tmp = shl <8 x i8> %a, <i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3>
  ret <8 x i8> %tmp
}

define <4 x i16> @test_shl_v4i16(<4 x i16> %a) {
; CHECK: test_shl_v4i16:
; CHECK: shl {{v[0-9]+}}.4h, {{v[0-9]+}}.4h, #3
  %tmp = shl <4 x i16> %a, <i16 3, i16 3, i16 3, i16 3>
  ret <4 x i16> %tmp
}

define <2 x i32> @test_shl_v2i32(<2 x i32> %a) {
; CHECK: test_shl_v2i32:
; CHECK: shl {{v[0-9]+}}.2s, {{v[0-9]+}}.2s, #3
  %tmp = shl <2 x i32> %a, <i32 3, i32 3>
  ret <2 x i32> %tmp
}

define <16 x i8> @test_shl_v16i8(<16 x i8> %a) {
; CHECK: test_shl_v16i8:
; CHECK: shl {{v[0-9]+}}.16b, {{v[0-9]+}}.16b, #3
  %tmp = shl <16 x i8> %a, <i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3>
  ret <16 x i8> %tmp
}

define <8 x i16> @test_shl_v8i16(<8 x i16> %a) {
; CHECK: test_shl_v8i16:
; CHECK: shl {{v[0-9]+}}.8h, {{v[0-9]+}}.8h, #3
  %tmp = shl <8 x i16> %a, <i16 3, i16 3, i16 3, i16 3, i16 3, i16 3, i16 3, i16 3>
  ret <8 x i16> %tmp
}

define <4 x i32> @test_shl_v4i32(<4 x i32> %a) {
; CHECK: test_shl_v4i32:
; CHECK: shl {{v[0-9]+}}.4s, {{v[0-9]+}}.4s, #3
  %tmp = shl <4 x i32> %a, <i32 3, i32 3, i32 3, i32 3>
  ret <4 x i32> %tmp
}

define <2 x i64> @test_shl_v2i64(<2 x i64> %a) {
; CHECK: test_shl_v2i64:
; CHECK: shl {{v[0-9]+}}.2d, {{v[0-9]+}}.2d, #63
  %tmp = shl <2 x i64> %a, <i64 63, i64 63>
  ret <2 x i64> %tmp
}


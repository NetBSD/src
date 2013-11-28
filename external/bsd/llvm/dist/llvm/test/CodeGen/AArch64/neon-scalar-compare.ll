; RUN: llc -mtriple=aarch64-none-linux-gnu -mattr=+neon < %s | FileCheck %s

;; Scalar Integer Compare

define i64 @test_vceqd(i64 %a, i64 %b) {
; CHECK: test_vceqd
; CHECK: cmeq {{d[0-9]+}}, {{d[0-9]}}, {{d[0-9]}}
entry:
  %vceq.i = insertelement <1 x i64> undef, i64 %a, i32 0
  %vceq1.i = insertelement <1 x i64> undef, i64 %b, i32 0
  %vceq2.i = call <1 x i64> @llvm.aarch64.neon.vceq.v1i64.v1i64.v1i64(<1 x i64> %vceq.i, <1 x i64> %vceq1.i)
  %0 = extractelement <1 x i64> %vceq2.i, i32 0
  ret i64 %0
}

define i64 @test_vceqzd(i64 %a) {
; CHECK: test_vceqzd
; CHECK: cmeq {{d[0-9]}}, {{d[0-9]}}, #0x0
entry:
  %vceqz.i = insertelement <1 x i64> undef, i64 %a, i32 0
  %vceqz1.i = call <1 x i64> @llvm.aarch64.neon.vceq.v1i64.v1i64.v1i64(<1 x i64> %vceqz.i, <1 x i64> zeroinitializer)
  %0 = extractelement <1 x i64> %vceqz1.i, i32 0
  ret i64 %0
}

define i64 @test_vcged(i64 %a, i64 %b) {
; CHECK: test_vcged
; CHECK: cmge {{d[0-9]}}, {{d[0-9]}}, {{d[0-9]}}
entry:
  %vcge.i = insertelement <1 x i64> undef, i64 %a, i32 0
  %vcge1.i = insertelement <1 x i64> undef, i64 %b, i32 0
  %vcge2.i = call <1 x i64> @llvm.aarch64.neon.vcge.v1i64.v1i64.v1i64(<1 x i64> %vcge.i, <1 x i64> %vcge1.i)
  %0 = extractelement <1 x i64> %vcge2.i, i32 0
  ret i64 %0
}

define i64 @test_vcgezd(i64 %a) {
; CHECK: test_vcgezd
; CHECK: cmge {{d[0-9]}}, {{d[0-9]}}, #0x0
entry:
  %vcgez.i = insertelement <1 x i64> undef, i64 %a, i32 0
  %vcgez1.i = call <1 x i64> @llvm.aarch64.neon.vcge.v1i64.v1i64.v1i64(<1 x i64> %vcgez.i, <1 x i64> zeroinitializer)
  %0 = extractelement <1 x i64> %vcgez1.i, i32 0
  ret i64 %0
}

define i64 @test_vcgtd(i64 %a, i64 %b) {
; CHECK: test_vcgtd
; CHECK: cmgt {{d[0-9]}}, {{d[0-9]}}, {{d[0-9]}}
entry:
  %vcgt.i = insertelement <1 x i64> undef, i64 %a, i32 0
  %vcgt1.i = insertelement <1 x i64> undef, i64 %b, i32 0
  %vcgt2.i = call <1 x i64> @llvm.aarch64.neon.vcgt.v1i64.v1i64.v1i64(<1 x i64> %vcgt.i, <1 x i64> %vcgt1.i)
  %0 = extractelement <1 x i64> %vcgt2.i, i32 0
  ret i64 %0
}

define i64 @test_vcgtzd(i64 %a) {
; CHECK: test_vcgtzd
; CHECK: cmgt {{d[0-9]}}, {{d[0-9]}}, #0x0
entry:
  %vcgtz.i = insertelement <1 x i64> undef, i64 %a, i32 0
  %vcgtz1.i = call <1 x i64> @llvm.aarch64.neon.vcgt.v1i64.v1i64.v1i64(<1 x i64> %vcgtz.i, <1 x i64> zeroinitializer)
  %0 = extractelement <1 x i64> %vcgtz1.i, i32 0
  ret i64 %0
}

define i64 @test_vcled(i64 %a, i64 %b) {
; CHECK: test_vcled
; CHECK: cmgt {{d[0-9]}}, {{d[0-9]}}, {{d[0-9]}}
entry:
  %vcgt.i = insertelement <1 x i64> undef, i64 %b, i32 0
  %vcgt1.i = insertelement <1 x i64> undef, i64 %a, i32 0
  %vcgt2.i = call <1 x i64> @llvm.aarch64.neon.vcgt.v1i64.v1i64.v1i64(<1 x i64> %vcgt.i, <1 x i64> %vcgt1.i)
  %0 = extractelement <1 x i64> %vcgt2.i, i32 0
  ret i64 %0
}

define i64 @test_vclezd(i64 %a) {
; CHECK: test_vclezd
; CHECK: cmle {{d[0-9]}}, {{d[0-9]}}, #0x0
entry:
  %vclez.i = insertelement <1 x i64> undef, i64 %a, i32 0
  %vclez1.i = call <1 x i64> @llvm.aarch64.neon.vclez.v1i64.v1i64.v1i64(<1 x i64> %vclez.i, <1 x i64> zeroinitializer)
  %0 = extractelement <1 x i64> %vclez1.i, i32 0
  ret i64 %0
}

define i64 @test_vcltd(i64 %a, i64 %b) {
; CHECK: test_vcltd
; CHECK: cmge {{d[0-9]}}, {{d[0-9]}}, {{d[0-9]}}
entry:
  %vcge.i = insertelement <1 x i64> undef, i64 %b, i32 0
  %vcge1.i = insertelement <1 x i64> undef, i64 %a, i32 0
  %vcge2.i = call <1 x i64> @llvm.aarch64.neon.vcge.v1i64.v1i64.v1i64(<1 x i64> %vcge.i, <1 x i64> %vcge1.i)
  %0 = extractelement <1 x i64> %vcge2.i, i32 0
  ret i64 %0
}

define i64 @test_vcltzd(i64 %a) {
; CHECK: test_vcltzd
; CHECK: cmlt {{d[0-9]}}, {{d[0-9]}}, #0x0
entry:
  %vcltz.i = insertelement <1 x i64> undef, i64 %a, i32 0
  %vcltz1.i = call <1 x i64> @llvm.aarch64.neon.vcltz.v1i64.v1i64.v1i64(<1 x i64> %vcltz.i, <1 x i64> zeroinitializer)
  %0 = extractelement <1 x i64> %vcltz1.i, i32 0
  ret i64 %0
}

define i64 @test_vtstd(i64 %a, i64 %b) {
; CHECK: test_vtstd
; CHECK: cmtst {{d[0-9]}}, {{d[0-9]}}, {{d[0-9]}}
entry:
  %vtst.i = insertelement <1 x i64> undef, i64 %a, i32 0
  %vtst1.i = insertelement <1 x i64> undef, i64 %b, i32 0
  %vtst2.i = call <1 x i64> @llvm.aarch64.neon.vtstd.v1i64.v1i64.v1i64(<1 x i64> %vtst.i, <1 x i64> %vtst1.i)
  %0 = extractelement <1 x i64> %vtst2.i, i32 0
  ret i64 %0
}

declare <1 x i64> @llvm.aarch64.neon.vtstd.v1i64.v1i64.v1i64(<1 x i64>, <1 x i64>)
declare <1 x i64> @llvm.aarch64.neon.vcltz.v1i64.v1i64.v1i64(<1 x i64>, <1 x i64>)
declare <1 x i64> @llvm.aarch64.neon.vchs.v1i64.v1i64.v1i64(<1 x i64>, <1 x i64>)
declare <1 x i64> @llvm.aarch64.neon.vcge.v1i64.v1i64.v1i64(<1 x i64>, <1 x i64>)
declare <1 x i64> @llvm.aarch64.neon.vclez.v1i64.v1i64.v1i64(<1 x i64>, <1 x i64>)
declare <1 x i64> @llvm.aarch64.neon.vchi.v1i64.v1i64.v1i64(<1 x i64>, <1 x i64>)
declare <1 x i64> @llvm.aarch64.neon.vcgt.v1i64.v1i64.v1i64(<1 x i64>, <1 x i64>)
declare <1 x i64> @llvm.aarch64.neon.vceq.v1i64.v1i64.v1i64(<1 x i64>, <1 x i64>)

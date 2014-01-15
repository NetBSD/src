; RUN: llc < %s -mtriple=x86_64-apple-darwin -mcpu=knl --show-mc-encoding| FileCheck %s

declare i32 @llvm.x86.avx512.kortestz.w(i16, i16) nounwind readnone
; CHECK-LABEL: test_kortestz
; CHECK: kortestw
; CHECK: sete
define i32 @test_kortestz(i16 %a0, i16 %a1) {
  %res = call i32 @llvm.x86.avx512.kortestz.w(i16 %a0, i16 %a1) 
  ret i32 %res
}

declare i32 @llvm.x86.avx512.kortestc.w(i16, i16) nounwind readnone
; CHECK-LABEL: test_kortestc
; CHECK: kortestw
; CHECK: sbbl
define i32 @test_kortestc(i16 %a0, i16 %a1) {
  %res = call i32 @llvm.x86.avx512.kortestc.w(i16 %a0, i16 %a1) 
  ret i32 %res
}

declare i16 @llvm.x86.avx512.kand.w(i16, i16) nounwind readnone
; CHECK-LABEL: test_kand
; CHECK: kandw
; CHECK: kandw
define i16 @test_kand(i16 %a0, i16 %a1) {
  %t1 = call i16 @llvm.x86.avx512.kand.w(i16 %a0, i16 8)
  %t2 = call i16 @llvm.x86.avx512.kand.w(i16 %t1, i16 %a1)
  ret i16 %t2
}

declare i16 @llvm.x86.avx512.knot.w(i16) nounwind readnone
; CHECK-LABEL: test_knot
; CHECK: knotw
define i16 @test_knot(i16 %a0) {
  %res = call i16 @llvm.x86.avx512.knot.w(i16 %a0)
  ret i16 %res
}

declare i16 @llvm.x86.avx512.kunpck.bw(i16, i16) nounwind readnone

; CHECK-LABEL: unpckbw_test
; CHECK: kunpckbw
; CHECK:ret
define i16 @unpckbw_test(i16 %a0, i16 %a1) {
  %res = call i16 @llvm.x86.avx512.kunpck.bw(i16 %a0, i16 %a1)
  ret i16 %res
}

define <16 x float> @test_rcp_ps_512(<16 x float> %a0) {
  ; CHECK: vrcp14ps {{.*}}encoding: [0x62,0xf2,0x7d,0x48,0x4c,0xc0]
  %res = call <16 x float> @llvm.x86.avx512.rcp14.ps.512(<16 x float> %a0, <16 x float> zeroinitializer, i16 -1) ; <<16 x float>> [#uses=1]
  ret <16 x float> %res
}
declare <16 x float> @llvm.x86.avx512.rcp14.ps.512(<16 x float>, <16 x float>, i16) nounwind readnone

define <8 x double> @test_rcp_pd_512(<8 x double> %a0) {
  ; CHECK: vrcp14pd {{.*}}encoding: [0x62,0xf2,0xfd,0x48,0x4c,0xc0]
  %res = call <8 x double> @llvm.x86.avx512.rcp14.pd.512(<8 x double> %a0, <8 x double> zeroinitializer, i8 -1) ; <<8 x double>> [#uses=1]
  ret <8 x double> %res
}
declare <8 x double> @llvm.x86.avx512.rcp14.pd.512(<8 x double>, <8 x double>, i8) nounwind readnone

define <16 x float> @test_rcp28_ps_512(<16 x float> %a0) {
  ; CHECK: vrcp28ps {sae}, {{.*}}encoding: [0x62,0xf2,0x7d,0x18,0xca,0xc0]
  %res = call <16 x float> @llvm.x86.avx512.rcp28.ps(<16 x float> %a0, <16 x float> zeroinitializer, i16 -1, i32 8) ; <<16 x float>> [#uses=1]
  ret <16 x float> %res
}
declare <16 x float> @llvm.x86.avx512.rcp28.ps(<16 x float>, <16 x float>, i16, i32) nounwind readnone

define <8 x double> @test_rcp28_pd_512(<8 x double> %a0) {
  ; CHECK: vrcp28pd {sae}, {{.*}}encoding: [0x62,0xf2,0xfd,0x18,0xca,0xc0]
  %res = call <8 x double> @llvm.x86.avx512.rcp28.pd(<8 x double> %a0, <8 x double> zeroinitializer, i8 -1, i32 8) ; <<8 x double>> [#uses=1]
  ret <8 x double> %res
}
declare <8 x double> @llvm.x86.avx512.rcp28.pd(<8 x double>, <8 x double>, i8, i32) nounwind readnone

declare <8 x double> @llvm.x86.avx512.mask.rndscale.pd.512(<8 x double>, i32, <8 x double>, i8, i32)

define <8 x double> @test7(<8 x double> %a) {
; CHECK: vrndscalepd {{.*}}encoding: [0x62,0xf3,0xfd,0x48,0x09,0xc0,0x0b]
  %res = call <8 x double> @llvm.x86.avx512.mask.rndscale.pd.512(<8 x double> %a, i32 11, <8 x double> zeroinitializer, i8 -1, i32 4)
  ret <8 x double>%res
}

declare <16 x float> @llvm.x86.avx512.mask.rndscale.ps.512(<16 x float>, i32, <16 x float>, i16, i32)

define <16 x float> @test8(<16 x float> %a) {
; CHECK: vrndscaleps {{.*}}encoding: [0x62,0xf3,0x7d,0x48,0x08,0xc0,0x0b]
  %res = call <16 x float> @llvm.x86.avx512.mask.rndscale.ps.512(<16 x float> %a, i32 11, <16 x float> zeroinitializer, i16 -1, i32 4)
  ret <16 x float>%res
}

define <16 x float> @test_rsqrt_ps_512(<16 x float> %a0) {
  ; CHECK: vrsqrt14ps {{.*}}encoding: [0x62,0xf2,0x7d,0x48,0x4e,0xc0]
  %res = call <16 x float> @llvm.x86.avx512.rsqrt14.ps.512(<16 x float> %a0, <16 x float> zeroinitializer, i16 -1) ; <<16 x float>> [#uses=1]
  ret <16 x float> %res
}
declare <16 x float> @llvm.x86.avx512.rsqrt14.ps.512(<16 x float>, <16 x float>, i16) nounwind readnone

define <16 x float> @test_rsqrt28_ps_512(<16 x float> %a0) {
  ; CHECK: vrsqrt28ps {sae}, {{.*}}encoding: [0x62,0xf2,0x7d,0x18,0xcc,0xc0]
  %res = call <16 x float> @llvm.x86.avx512.rsqrt28.ps(<16 x float> %a0, <16 x float> zeroinitializer, i16 -1, i32 8) ; <<16 x float>> [#uses=1]
  ret <16 x float> %res
}
declare <16 x float> @llvm.x86.avx512.rsqrt28.ps(<16 x float>, <16 x float>, i16, i32) nounwind readnone

define <4 x float> @test_rsqrt14_ss(<4 x float> %a0) {
  ; CHECK: vrsqrt14ss {{.*}}encoding: [0x62,0xf2,0x7d,0x08,0x4f,0xc0]
  %res = call <4 x float> @llvm.x86.avx512.rsqrt14.ss(<4 x float> %a0, <4 x float> %a0, <4 x float> zeroinitializer, i8 -1) ; <<4 x float>> [#uses=1]
  ret <4 x float> %res
}
declare <4 x float> @llvm.x86.avx512.rsqrt14.ss(<4 x float>, <4 x float>, <4 x float>, i8) nounwind readnone

define <4 x float> @test_rsqrt28_ss(<4 x float> %a0) {
  ; CHECK: vrsqrt28ss {sae}, {{.*}}encoding: [0x62,0xf2,0x7d,0x18,0xcd,0xc0]
  %res = call <4 x float> @llvm.x86.avx512.rsqrt28.ss(<4 x float> %a0, <4 x float> %a0, <4 x float> zeroinitializer, i8 -1, i32 8) ; <<4 x float>> [#uses=1]
  ret <4 x float> %res
}
declare <4 x float> @llvm.x86.avx512.rsqrt28.ss(<4 x float>, <4 x float>, <4 x float>, i8, i32) nounwind readnone

define <4 x float> @test_rcp14_ss(<4 x float> %a0) {
  ; CHECK: vrcp14ss {{.*}}encoding: [0x62,0xf2,0x7d,0x08,0x4d,0xc0]
  %res = call <4 x float> @llvm.x86.avx512.rcp14.ss(<4 x float> %a0, <4 x float> %a0, <4 x float> zeroinitializer, i8 -1) ; <<4 x float>> [#uses=1]
  ret <4 x float> %res
}
declare <4 x float> @llvm.x86.avx512.rcp14.ss(<4 x float>, <4 x float>, <4 x float>, i8) nounwind readnone

define <4 x float> @test_rcp28_ss(<4 x float> %a0) {
  ; CHECK: vrcp28ss {sae}, {{.*}}encoding: [0x62,0xf2,0x7d,0x18,0xcb,0xc0]
  %res = call <4 x float> @llvm.x86.avx512.rcp28.ss(<4 x float> %a0, <4 x float> %a0, <4 x float> zeroinitializer, i8 -1, i32 8) ; <<4 x float>> [#uses=1]
  ret <4 x float> %res
}
declare <4 x float> @llvm.x86.avx512.rcp28.ss(<4 x float>, <4 x float>, <4 x float>, i8, i32) nounwind readnone

define <8 x double> @test_sqrt_pd_512(<8 x double> %a0) {
  ; CHECK: vsqrtpd
  %res = call <8 x double> @llvm.x86.avx512.sqrt.pd.512(<8 x double> %a0) ; <<8 x double>> [#uses=1]
  ret <8 x double> %res
}
declare <8 x double> @llvm.x86.avx512.sqrt.pd.512(<8 x double>) nounwind readnone

define <16 x float> @test_sqrt_ps_512(<16 x float> %a0) {
  ; CHECK: vsqrtps
  %res = call <16 x float> @llvm.x86.avx512.sqrt.ps.512(<16 x float> %a0) ; <<16 x float>> [#uses=1]
  ret <16 x float> %res
}
declare <16 x float> @llvm.x86.avx512.sqrt.ps.512(<16 x float>) nounwind readnone

define <4 x float> @test_sqrt_ss(<4 x float> %a0, <4 x float> %a1) {
  ; CHECK: vsqrtss {{.*}}encoding: [0x62
  %res = call <4 x float> @llvm.x86.avx512.sqrt.ss(<4 x float> %a0, <4 x float> %a1) ; <<4 x float>> [#uses=1]
  ret <4 x float> %res
}
declare <4 x float> @llvm.x86.avx512.sqrt.ss(<4 x float>, <4 x float>) nounwind readnone

define <2 x double> @test_sqrt_sd(<2 x double> %a0, <2 x double> %a1) {
  ; CHECK: vsqrtsd {{.*}}encoding: [0x62
  %res = call <2 x double> @llvm.x86.avx512.sqrt.sd(<2 x double> %a0, <2 x double> %a1) ; <<2 x double>> [#uses=1]
  ret <2 x double> %res
}
declare <2 x double> @llvm.x86.avx512.sqrt.sd(<2 x double>, <2 x double>) nounwind readnone

define i64 @test_x86_sse2_cvtsd2si64(<2 x double> %a0) {
  ; CHECK: vcvtsd2si {{.*}}encoding: [0x62
  %res = call i64 @llvm.x86.sse2.cvtsd2si64(<2 x double> %a0) ; <i64> [#uses=1]
  ret i64 %res
}
declare i64 @llvm.x86.sse2.cvtsd2si64(<2 x double>) nounwind readnone

define <2 x double> @test_x86_sse2_cvtsi642sd(<2 x double> %a0, i64 %a1) {
  ; CHECK: vcvtsi2sdq {{.*}}encoding: [0x62
  %res = call <2 x double> @llvm.x86.sse2.cvtsi642sd(<2 x double> %a0, i64 %a1) ; <<2 x double>> [#uses=1]
  ret <2 x double> %res
}
declare <2 x double> @llvm.x86.sse2.cvtsi642sd(<2 x double>, i64) nounwind readnone

define <2 x double> @test_x86_avx512_cvtusi642sd(<2 x double> %a0, i64 %a1) {
  ; CHECK: vcvtusi2sdq {{.*}}encoding: [0x62
  %res = call <2 x double> @llvm.x86.avx512.cvtusi642sd(<2 x double> %a0, i64 %a1) ; <<2 x double>> [#uses=1]
  ret <2 x double> %res
}
declare <2 x double> @llvm.x86.avx512.cvtusi642sd(<2 x double>, i64) nounwind readnone

define i64 @test_x86_sse2_cvttsd2si64(<2 x double> %a0) {
  ; CHECK: vcvttsd2si {{.*}}encoding: [0x62
  %res = call i64 @llvm.x86.sse2.cvttsd2si64(<2 x double> %a0) ; <i64> [#uses=1]
  ret i64 %res
}
declare i64 @llvm.x86.sse2.cvttsd2si64(<2 x double>) nounwind readnone


define i64 @test_x86_sse_cvtss2si64(<4 x float> %a0) {
  ; CHECK: vcvtss2si {{.*}}encoding: [0x62
  %res = call i64 @llvm.x86.sse.cvtss2si64(<4 x float> %a0) ; <i64> [#uses=1]
  ret i64 %res
}
declare i64 @llvm.x86.sse.cvtss2si64(<4 x float>) nounwind readnone


define <4 x float> @test_x86_sse_cvtsi642ss(<4 x float> %a0, i64 %a1) {
  ; CHECK: vcvtsi2ssq {{.*}}encoding: [0x62
  %res = call <4 x float> @llvm.x86.sse.cvtsi642ss(<4 x float> %a0, i64 %a1) ; <<4 x float>> [#uses=1]
  ret <4 x float> %res
}
declare <4 x float> @llvm.x86.sse.cvtsi642ss(<4 x float>, i64) nounwind readnone


define i64 @test_x86_sse_cvttss2si64(<4 x float> %a0) {
  ; CHECK: vcvttss2si {{.*}}encoding: [0x62
  %res = call i64 @llvm.x86.sse.cvttss2si64(<4 x float> %a0) ; <i64> [#uses=1]
  ret i64 %res
}
declare i64 @llvm.x86.sse.cvttss2si64(<4 x float>) nounwind readnone

define i64 @test_x86_avx512_cvtsd2usi64(<2 x double> %a0) {
  ; CHECK: vcvtsd2usi {{.*}}encoding: [0x62
  %res = call i64 @llvm.x86.avx512.cvtsd2usi64(<2 x double> %a0) ; <i64> [#uses=1]
  ret i64 %res
}
declare i64 @llvm.x86.avx512.cvtsd2usi64(<2 x double>) nounwind readnone

define <16 x float> @test_x86_vcvtph2ps_512(<16 x i16> %a0) {
  ; CHECK: vcvtph2ps
  %res = call <16 x float> @llvm.x86.avx512.vcvtph2ps.512(<16 x i16> %a0)
  ret <16 x float> %res
}
declare <16 x float> @llvm.x86.avx512.vcvtph2ps.512(<16 x i16>) nounwind readonly


define <16 x i16> @test_x86_vcvtps2ph_256(<16 x float> %a0) {
  ; CHECK: vcvtps2ph
  %res = call <16 x i16> @llvm.x86.avx512.vcvtps2ph.512(<16 x float> %a0, i32 0)
  ret <16 x i16> %res
}
declare <16 x i16> @llvm.x86.avx512.vcvtps2ph.512(<16 x float>, i32) nounwind readonly

define <16 x float> @test_x86_vbroadcast_ss_512(i8* %a0) {
  ; CHECK: vbroadcastss
  %res = call <16 x float> @llvm.x86.avx512.vbroadcast.ss.512(i8* %a0) ; <<16 x float>> [#uses=1]
  ret <16 x float> %res
}
declare <16 x float> @llvm.x86.avx512.vbroadcast.ss.512(i8*) nounwind readonly

define <8 x double> @test_x86_vbroadcast_sd_512(i8* %a0) {
  ; CHECK: vbroadcastsd
  %res = call <8 x double> @llvm.x86.avx512.vbroadcast.sd.512(i8* %a0) ; <<8 x double>> [#uses=1]
  ret <8 x double> %res
}
declare <8 x double> @llvm.x86.avx512.vbroadcast.sd.512(i8*) nounwind readonly

define <16 x float> @test_x86_vbroadcast_ss_ps_512(<4 x float> %a0) {
  ; CHECK: vbroadcastss
  %res = call <16 x float> @llvm.x86.avx512.vbroadcast.ss.ps.512(<4 x float> %a0) ; <<16 x float>> [#uses=1]
  ret <16 x float> %res
}
declare <16 x float> @llvm.x86.avx512.vbroadcast.ss.ps.512(<4 x float>) nounwind readonly

define <8 x double> @test_x86_vbroadcast_sd_pd_512(<2 x double> %a0) {
  ; CHECK: vbroadcastsd
  %res = call <8 x double> @llvm.x86.avx512.vbroadcast.sd.pd.512(<2 x double> %a0) ; <<8 x double>> [#uses=1]
  ret <8 x double> %res
}
declare <8 x double> @llvm.x86.avx512.vbroadcast.sd.pd.512(<2 x double>) nounwind readonly

define <16 x i32> @test_x86_pbroadcastd_512(<4 x i32>  %a0) {
  ; CHECK: vpbroadcastd
  %res = call <16 x i32> @llvm.x86.avx512.pbroadcastd.512(<4 x i32> %a0) ; <<16 x i32>> [#uses=1]
  ret <16 x i32> %res
}
declare <16 x i32> @llvm.x86.avx512.pbroadcastd.512(<4 x i32>) nounwind readonly

define <16 x i32> @test_x86_pbroadcastd_i32_512(i32  %a0) {
  ; CHECK: vpbroadcastd
  %res = call <16 x i32> @llvm.x86.avx512.pbroadcastd.i32.512(i32 %a0) ; <<16 x i32>> [#uses=1]
  ret <16 x i32> %res
}
declare <16 x i32> @llvm.x86.avx512.pbroadcastd.i32.512(i32) nounwind readonly

define <8 x i64> @test_x86_pbroadcastq_512(<2 x i64> %a0) {
  ; CHECK: vpbroadcastq
  %res = call <8 x i64> @llvm.x86.avx512.pbroadcastq.512(<2 x i64> %a0) ; <<8 x i64>> [#uses=1]
  ret <8 x i64> %res
}
declare <8 x i64> @llvm.x86.avx512.pbroadcastq.512(<2 x i64>) nounwind readonly

define <8 x i64> @test_x86_pbroadcastq_i64_512(i64 %a0) {
  ; CHECK: vpbroadcastq
  %res = call <8 x i64> @llvm.x86.avx512.pbroadcastq.i64.512(i64 %a0) ; <<8 x i64>> [#uses=1]
  ret <8 x i64> %res
}
declare <8 x i64> @llvm.x86.avx512.pbroadcastq.i64.512(i64) nounwind readonly

define <16 x i32> @test_conflict_d(<16 x i32> %a) {
  ; CHECK: movw $-1, %ax
  ; CHECK: vpxor
  ; CHECK: vpconflictd
  %res = call <16 x i32> @llvm.x86.avx512.mask.conflict.d.512(<16 x i32> %a, <16 x i32> zeroinitializer, i16 -1)
  ret <16 x i32> %res
}

declare <16 x i32> @llvm.x86.avx512.mask.conflict.d.512(<16 x i32>, <16 x i32>, i16) nounwind readonly

define <8 x i64> @test_conflict_q(<8 x i64> %a) {
  ; CHECK: movb $-1, %al
  ; CHECK: vpxor
  ; CHECK: vpconflictq
  %res = call <8 x i64> @llvm.x86.avx512.mask.conflict.q.512(<8 x i64> %a, <8 x i64> zeroinitializer, i8 -1)
  ret <8 x i64> %res
}

declare <8 x i64> @llvm.x86.avx512.mask.conflict.q.512(<8 x i64>, <8 x i64>, i8) nounwind readonly


define <16 x i32> @test_maskz_conflict_d(<16 x i32> %a, i16 %mask) {
  ; CHECK: vpconflictd 
  %res = call <16 x i32> @llvm.x86.avx512.mask.conflict.d.512(<16 x i32> %a, <16 x i32> zeroinitializer, i16 %mask)
  ret <16 x i32> %res
}

define <8 x i64> @test_mask_conflict_q(<8 x i64> %a, <8 x i64> %b, i8 %mask) {
  ; CHECK: vpconflictq
  %res = call <8 x i64> @llvm.x86.avx512.mask.conflict.q.512(<8 x i64> %a, <8 x i64> %b, i8 %mask)
  ret <8 x i64> %res
}

define <16 x float> @test_x86_mask_blend_ps_512(i16 %a0, <16 x float> %a1, <16 x float> %a2) {
  ; CHECK: vblendmps
  %res = call <16 x float> @llvm.x86.avx512.mask.blend.ps.512(<16 x float> %a1, <16 x float> %a2, i16 %a0) ; <<16 x float>> [#uses=1]
  ret <16 x float> %res
}

declare <16 x float> @llvm.x86.avx512.mask.blend.ps.512(<16 x float>, <16 x float>, i16) nounwind readonly

define <8 x double> @test_x86_mask_blend_pd_512(i8 %a0, <8 x double> %a1, <8 x double> %a2) {
  ; CHECK: vblendmpd
  %res = call <8 x double> @llvm.x86.avx512.mask.blend.pd.512(<8 x double> %a1, <8 x double> %a2, i8 %a0) ; <<8 x double>> [#uses=1]
  ret <8 x double> %res
}

define <8 x double> @test_x86_mask_blend_pd_512_memop(<8 x double> %a, <8 x double>* %ptr, i8 %mask) {
  ; CHECK-LABEL: test_x86_mask_blend_pd_512_memop
  ; CHECK: vblendmpd (%
  %b = load <8 x double>* %ptr
  %res = call <8 x double> @llvm.x86.avx512.mask.blend.pd.512(<8 x double> %a, <8 x double> %b, i8 %mask) ; <<8 x double>> [#uses=1]
  ret <8 x double> %res
}
declare <8 x double> @llvm.x86.avx512.mask.blend.pd.512(<8 x double>, <8 x double>, i8) nounwind readonly

define <16 x i32> @test_x86_mask_blend_d_512(i16 %a0, <16 x i32> %a1, <16 x i32> %a2) {
  ; CHECK: vpblendmd
  %res = call <16 x i32> @llvm.x86.avx512.mask.blend.d.512(<16 x i32> %a1, <16 x i32> %a2, i16 %a0) ; <<16 x i32>> [#uses=1]
  ret <16 x i32> %res
}
declare <16 x i32> @llvm.x86.avx512.mask.blend.d.512(<16 x i32>, <16 x i32>, i16) nounwind readonly

define <8 x i64> @test_x86_mask_blend_q_512(i8 %a0, <8 x i64> %a1, <8 x i64> %a2) {
  ; CHECK: vpblendmq
  %res = call <8 x i64> @llvm.x86.avx512.mask.blend.q.512(<8 x i64> %a1, <8 x i64> %a2, i8 %a0) ; <<8 x i64>> [#uses=1]
  ret <8 x i64> %res
}
declare <8 x i64> @llvm.x86.avx512.mask.blend.q.512(<8 x i64>, <8 x i64>, i8) nounwind readonly

 define <8 x i32> @test_cvtpd2udq(<8 x double> %a) {
 ;CHECK: vcvtpd2udq {ru-sae}{{.*}}encoding: [0x62,0xf1,0xfc,0x58,0x79,0xc0]
  %res = call <8 x i32> @llvm.x86.avx512.mask.cvtpd2udq.512(<8 x double> %a, <8 x i32>zeroinitializer, i8 -1, i32 2)
  ret <8 x i32>%res
 }
 declare <8 x i32> @llvm.x86.avx512.mask.cvtpd2udq.512(<8 x double>, <8 x i32>, i8, i32)
 
 define <16 x i32> @test_cvtps2udq(<16 x float> %a) {
 ;CHECK: vcvtps2udq {rd-sae}{{.*}}encoding: [0x62,0xf1,0x7c,0x38,0x79,0xc0]
  %res = call <16 x i32> @llvm.x86.avx512.mask.cvtps2udq.512(<16 x float> %a, <16 x i32>zeroinitializer, i16 -1, i32 1)
  ret <16 x i32>%res
 }
 declare <16 x i32> @llvm.x86.avx512.mask.cvtps2udq.512(<16 x float>, <16 x i32>, i16, i32)

 define i16 @test_cmpps(<16 x float> %a, <16 x float> %b) {
 ;CHECK: vcmpleps {sae}{{.*}}encoding: [0x62,0xf1,0x7c,0x18,0xc2,0xc1,0x02]
   %res = call i16 @llvm.x86.avx512.mask.cmp.ps.512(<16 x float> %a, <16 x float> %b, i32 2, i16 -1, i32 8)
   ret i16 %res
 }
 declare i16 @llvm.x86.avx512.mask.cmp.ps.512(<16 x float> , <16 x float> , i32, i16, i32)

 define i8 @test_cmppd(<8 x double> %a, <8 x double> %b) {
 ;CHECK: vcmpneqpd %zmm{{.*}}encoding: [0x62,0xf1,0xfd,0x48,0xc2,0xc1,0x04]
   %res = call i8 @llvm.x86.avx512.mask.cmp.pd.512(<8 x double> %a, <8 x double> %b, i32 4, i8 -1, i32 4)
   ret i8 %res
 }
 declare i8 @llvm.x86.avx512.mask.cmp.pd.512(<8 x double> , <8 x double> , i32, i8, i32)

 ; cvt intrinsics
 define <16 x float> @test_cvtdq2ps(<16 x i32> %a) {
 ;CHECK: vcvtdq2ps {rd-sae}{{.*}}encoding: [0x62,0xf1,0x7c,0x38,0x5b,0xc0]
  %res = call <16 x float> @llvm.x86.avx512.mask.cvtdq2ps.512(<16 x i32> %a, <16 x float>zeroinitializer, i16 -1, i32 1)
  ret <16 x float>%res
 }
 declare <16 x float> @llvm.x86.avx512.mask.cvtdq2ps.512(<16 x i32>, <16 x float>, i16, i32)

 define <16 x float> @test_cvtudq2ps(<16 x i32> %a) {
 ;CHECK: vcvtudq2ps {rd-sae}{{.*}}encoding: [0x62,0xf1,0x7f,0x38,0x7a,0xc0]
  %res = call <16 x float> @llvm.x86.avx512.mask.cvtudq2ps.512(<16 x i32> %a, <16 x float>zeroinitializer, i16 -1, i32 1)
  ret <16 x float>%res
 }
 declare <16 x float> @llvm.x86.avx512.mask.cvtudq2ps.512(<16 x i32>, <16 x float>, i16, i32)

 define <8 x double> @test_cvtdq2pd(<8 x i32> %a) {
 ;CHECK: vcvtdq2pd {{.*}}encoding: [0x62,0xf1,0x7e,0x48,0xe6,0xc0]
  %res = call <8 x double> @llvm.x86.avx512.mask.cvtdq2pd.512(<8 x i32> %a, <8 x double>zeroinitializer, i8 -1)
  ret <8 x double>%res
 }
 declare <8 x double> @llvm.x86.avx512.mask.cvtdq2pd.512(<8 x i32>, <8 x double>, i8)

 define <8 x double> @test_cvtudq2pd(<8 x i32> %a) {
 ;CHECK: vcvtudq2pd {{.*}}encoding: [0x62,0xf1,0x7e,0x48,0x7a,0xc0]
  %res = call <8 x double> @llvm.x86.avx512.mask.cvtudq2pd.512(<8 x i32> %a, <8 x double>zeroinitializer, i8 -1)
  ret <8 x double>%res
 }
 declare <8 x double> @llvm.x86.avx512.mask.cvtudq2pd.512(<8 x i32>, <8 x double>, i8)

 ; fp min - max
define <16 x float> @test_vmaxps(<16 x float> %a0, <16 x float> %a1) {
  ; CHECK: vmaxps
  %res = call <16 x float> @llvm.x86.avx512.mask.max.ps.512(<16 x float> %a0, <16 x float> %a1,
                    <16 x float>zeroinitializer, i16 -1, i32 4)
  ret <16 x float> %res
}
declare <16 x float> @llvm.x86.avx512.mask.max.ps.512(<16 x float>, <16 x float>,
                    <16 x float>, i16, i32)

define <8 x double> @test_vmaxpd(<8 x double> %a0, <8 x double> %a1) {
  ; CHECK: vmaxpd
  %res = call <8 x double> @llvm.x86.avx512.mask.max.pd.512(<8 x double> %a0, <8 x double> %a1,
                    <8 x double>zeroinitializer, i8 -1, i32 4)
  ret <8 x double> %res
}
declare <8 x double> @llvm.x86.avx512.mask.max.pd.512(<8 x double>, <8 x double>,
                    <8 x double>, i8, i32)

define <16 x float> @test_vminps(<16 x float> %a0, <16 x float> %a1) {
  ; CHECK: vminps
  %res = call <16 x float> @llvm.x86.avx512.mask.min.ps.512(<16 x float> %a0, <16 x float> %a1,
                    <16 x float>zeroinitializer, i16 -1, i32 4)
  ret <16 x float> %res
}
declare <16 x float> @llvm.x86.avx512.mask.min.ps.512(<16 x float>, <16 x float>,
                    <16 x float>, i16, i32)

define <8 x double> @test_vminpd(<8 x double> %a0, <8 x double> %a1) {
  ; CHECK: vminpd
  %res = call <8 x double> @llvm.x86.avx512.mask.min.pd.512(<8 x double> %a0, <8 x double> %a1,
                    <8 x double>zeroinitializer, i8 -1, i32 4)
  ret <8 x double> %res
}
declare <8 x double> @llvm.x86.avx512.mask.min.pd.512(<8 x double>, <8 x double>,
                    <8 x double>, i8, i32)

 define <8 x float> @test_cvtpd2ps(<8 x double> %a) {
 ;CHECK: vcvtpd2ps {rd-sae}{{.*}}encoding: [0x62,0xf1,0xfd,0x38,0x5a,0xc0]
  %res = call <8 x float> @llvm.x86.avx512.mask.cvtpd2ps.512(<8 x double> %a, <8 x float>zeroinitializer, i8 -1, i32 1)
  ret <8 x float>%res
 }
 declare <8 x float> @llvm.x86.avx512.mask.cvtpd2ps.512(<8 x double>, <8 x float>, i8, i32)

 define <16 x i32> @test_pabsd(<16 x i32> %a) {
 ;CHECK: vpabsd {{.*}}encoding: [0x62,0xf2,0x7d,0x48,0x1e,0xc0]
 %res = call <16 x i32> @llvm.x86.avx512.mask.pabs.d.512(<16 x i32> %a, <16 x i32>zeroinitializer, i16 -1)
 ret < 16 x i32> %res
 }
 declare <16 x i32> @llvm.x86.avx512.mask.pabs.d.512(<16 x i32>, <16 x i32>, i16)

 define <8 x i64> @test_pabsq(<8 x i64> %a) {
 ;CHECK: vpabsq {{.*}}encoding: [0x62,0xf2,0xfd,0x48,0x1f,0xc0]
 %res = call <8 x i64> @llvm.x86.avx512.mask.pabs.q.512(<8 x i64> %a, <8 x i64>zeroinitializer, i8 -1)
 ret <8 x i64> %res
 }
 declare <8 x i64> @llvm.x86.avx512.mask.pabs.q.512(<8 x i64>, <8 x i64>, i8)

define <8 x i64> @test_vpmaxq(<8 x i64> %a0, <8 x i64> %a1) {
  ; CHECK: vpmaxsq {{.*}}encoding: [0x62,0xf2,0xfd,0x48,0x3d,0xc1]
  %res = call <8 x i64> @llvm.x86.avx512.mask.pmaxs.q.512(<8 x i64> %a0, <8 x i64> %a1,
                    <8 x i64>zeroinitializer, i8 -1)
  ret <8 x i64> %res
}
declare <8 x i64> @llvm.x86.avx512.mask.pmaxs.q.512(<8 x i64>, <8 x i64>, <8 x i64>, i8)

define <16 x i32> @test_vpminud(<16 x i32> %a0, <16 x i32> %a1) {
  ; CHECK: vpminud {{.*}}encoding: [0x62,0xf2,0x7d,0x48,0x3b,0xc1]
  %res = call <16 x i32> @llvm.x86.avx512.mask.pminu.d.512(<16 x i32> %a0, <16 x i32> %a1,
                    <16 x i32>zeroinitializer, i16 -1)
  ret <16 x i32> %res
}
declare <16 x i32> @llvm.x86.avx512.mask.pminu.d.512(<16 x i32>, <16 x i32>, <16 x i32>, i16)

define <16 x i32> @test_vpmaxsd(<16 x i32> %a0, <16 x i32> %a1) {
  ; CHECK: vpmaxsd {{.*}}encoding: [0x62,0xf2,0x7d,0x48,0x3d,0xc1]
  %res = call <16 x i32> @llvm.x86.avx512.mask.pmaxs.d.512(<16 x i32> %a0, <16 x i32> %a1,
                    <16 x i32>zeroinitializer, i16 -1)
  ret <16 x i32> %res
}
declare <16 x i32> @llvm.x86.avx512.mask.pmaxs.d.512(<16 x i32>, <16 x i32>, <16 x i32>, i16)

define <8 x i64> @test_vpmuludq(<16 x i32> %a0, <16 x i32> %a1) {
  ; CHECK: vpmuludq {{.*}}encoding: [0x62,0xf1,0xfd,0x48,0xf4,0xc1]
  %res = call <8 x i64> @llvm.x86.avx512.mask.pmulu.dq.512(<16 x i32> %a0, <16 x i32> %a1,
                    <8 x i64>zeroinitializer, i8 -1)
  ret <8 x i64> %res
}
declare <8 x i64> @llvm.x86.avx512.mask.pmulu.dq.512(<16 x i32>, <16 x i32>, <8 x i64>, i8)

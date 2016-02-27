; RUN: llc < %s -mtriple=x86_64-apple-darwin -mattr=+avx512f --show-mc-encoding | FileCheck %s

declare <16 x float> @llvm.x86.avx512.mask.vfmadd.ps.512(<16 x float>, <16 x float>, <16 x float>, i16, i32)
declare <8 x double> @llvm.x86.avx512.mask.vfmadd.pd.512(<8 x double>, <8 x double>, <8 x double>, i8, i32)

define <16 x float> @test_x86_vfnmadd_ps_z(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2) {
  ; CHECK-LABEL: test_x86_vfnmadd_ps_z
  ; CHECK: vfnmadd213ps %zmm
  %res = call <16 x float> @llvm.x86.avx512.mask.vfnmadd.ps.512(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 -1, i32 4) nounwind
  ret <16 x float> %res
}
declare <16 x float> @llvm.x86.avx512.mask.vfnmadd.ps.512(<16 x float>, <16 x float>, <16 x float>, i16, i32) nounwind readnone

define <16 x float> @test_mask_vfnmadd_ps(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 %mask) {
  ; CHECK-LABEL: test_mask_vfnmadd_ps
  ; CHECK: vfnmadd213ps %zmm
  %res = call <16 x float> @llvm.x86.avx512.mask.vfnmadd.ps.512(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 %mask, i32 4) nounwind
  ret <16 x float> %res
}

define <8 x double> @test_x86_vfnmadd_pd_z(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2) {
  ; CHECK-LABEL: test_x86_vfnmadd_pd_z
  ; CHECK: vfnmadd213pd %zmm
  %res = call <8 x double> @llvm.x86.avx512.mask.vfnmadd.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 -1, i32 4) nounwind
  ret <8 x double> %res
}
declare <8 x double> @llvm.x86.avx512.mask.vfnmadd.pd.512(<8 x double>, <8 x double>, <8 x double>, i8, i32) nounwind readnone

define <8 x double> @test_mask_vfnmadd_pd(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask) {
  ; CHECK-LABEL: test_mask_vfnmadd_pd
  ; CHECK: vfnmadd213pd %zmm
  %res = call <8 x double> @llvm.x86.avx512.mask.vfnmadd.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask, i32 4) nounwind
  ret <8 x double> %res
}

define <16 x float> @test_x86_vfnmsubps_z(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2) {
  ; CHECK-LABEL: test_x86_vfnmsubps_z
  ; CHECK: vfnmsub213ps %zmm
  %res = call <16 x float> @llvm.x86.avx512.mask.vfnmsub.ps.512(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 -1, i32 4) nounwind
  ret <16 x float> %res
}
declare <16 x float> @llvm.x86.avx512.mask.vfnmsub.ps.512(<16 x float>, <16 x float>, <16 x float>, i16, i32) nounwind readnone

define <16 x float> @test_mask_vfnmsub_ps(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 %mask) {
  ; CHECK-LABEL: test_mask_vfnmsub_ps
  ; CHECK: vfnmsub213ps %zmm
  %res = call <16 x float> @llvm.x86.avx512.mask.vfnmsub.ps.512(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 %mask, i32 4) nounwind
  ret <16 x float> %res
}

define <8 x double> @test_x86_vfnmsubpd_z(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2) {
  ; CHECK-LABEL: test_x86_vfnmsubpd_z
  ; CHECK: vfnmsub213pd %zmm
  %res = call <8 x double> @llvm.x86.avx512.mask.vfnmsub.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 -1, i32 4) nounwind
  ret <8 x double> %res
}
declare <8 x double> @llvm.x86.avx512.mask.vfnmsub.pd.512(<8 x double>, <8 x double>, <8 x double>, i8, i32) nounwind readnone

define <8 x double> @test_mask_vfnmsub_pd(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask) {
  ; CHECK-LABEL: test_mask_vfnmsub_pd
  ; CHECK: vfnmsub213pd %zmm
  %res = call <8 x double> @llvm.x86.avx512.mask.vfnmsub.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask, i32 4) nounwind
  ret <8 x double> %res
}

define <16 x float> @test_x86_vfmaddsubps_z(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2) {
  ; CHECK-LABEL: test_x86_vfmaddsubps_z
  ; CHECK: vfmaddsub213ps %zmm
  %res = call <16 x float> @llvm.x86.avx512.mask.vfmaddsub.ps.512(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 -1, i32 4) nounwind
  ret <16 x float> %res
}

define <16 x float> @test_mask_fmaddsub_ps(<16 x float> %a, <16 x float> %b, <16 x float> %c, i16 %mask) {
; CHECK-LABEL: test_mask_fmaddsub_ps:
; CHECK: vfmaddsub213ps %zmm2, %zmm1, %zmm0 {%k1} ## encoding: [0x62,0xf2,0x75,0x49,0xa6,0xc2]
  %res = call <16 x float> @llvm.x86.avx512.mask.vfmaddsub.ps.512(<16 x float> %a, <16 x float> %b, <16 x float> %c, i16 %mask, i32 4)
  ret <16 x float> %res
}

declare <16 x float> @llvm.x86.avx512.mask.vfmaddsub.ps.512(<16 x float>, <16 x float>, <16 x float>, i16, i32) nounwind readnone

define <8 x double> @test_x86_vfmaddsubpd_z(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2) {
  ; CHECK-LABEL: test_x86_vfmaddsubpd_z
  ; CHECK: vfmaddsub213pd %zmm
  %res = call <8 x double> @llvm.x86.avx512.mask.vfmaddsub.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 -1, i32 4) nounwind
  ret <8 x double> %res
}
declare <8 x double> @llvm.x86.avx512.mask.vfmaddsub.pd.512(<8 x double>, <8 x double>, <8 x double>, i8, i32) nounwind readnone

define <8 x double> @test_mask_vfmaddsub_pd(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask) {
  ; CHECK-LABEL: test_mask_vfmaddsub_pd
  ; CHECK: vfmaddsub213pd %zmm
  %res = call <8 x double> @llvm.x86.avx512.mask.vfmaddsub.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask, i32 4) nounwind
  ret <8 x double> %res
}

define <8 x double>@test_int_x86_avx512_mask_vfmaddsub_pd_512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3){
; CHECK-LABEL: test_int_x86_avx512_mask_vfmaddsub_pd_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movzbl %dil, %eax
; CHECK-NEXT:    kmovw %eax, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm3
; CHECK-NEXT:    vfmaddsub213pd %zmm2, %zmm1, %zmm3 {%k1}
; CHECK-NEXT:    vfmaddsub213pd {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddpd %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <8 x double> @llvm.x86.avx512.mask.vfmaddsub.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3, i32 4)
  %res1 = call <8 x double> @llvm.x86.avx512.mask.vfmaddsub.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 -1, i32 0)
  %res2 = fadd <8 x double> %res, %res1
  ret <8 x double> %res2
}

declare <8 x double> @llvm.x86.avx512.mask3.vfmaddsub.pd.512(<8 x double>, <8 x double>, <8 x double>, i8, i32)

define <8 x double>@test_int_x86_avx512_mask3_vfmaddsub_pd_512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3){
; CHECK-LABEL: test_int_x86_avx512_mask3_vfmaddsub_pd_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movzbl %dil, %eax
; CHECK-NEXT:    kmovw %eax, %k1
; CHECK-NEXT:    vmovaps %zmm2, %zmm3
; CHECK-NEXT:    vfmaddsub231pd %zmm1, %zmm0, %zmm3 {%k1}
; CHECK-NEXT:    vfmaddsub213pd {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddpd %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <8 x double> @llvm.x86.avx512.mask3.vfmaddsub.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3, i32 4)
  %res1 = call <8 x double> @llvm.x86.avx512.mask3.vfmaddsub.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 -1, i32 0)
  %res2 = fadd <8 x double> %res, %res1
  ret <8 x double> %res2
}

declare <8 x double> @llvm.x86.avx512.maskz.vfmaddsub.pd.512(<8 x double>, <8 x double>, <8 x double>, i8, i32)

define <8 x double>@test_int_x86_avx512_maskz_vfmaddsub_pd_512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3){
; CHECK-LABEL: test_int_x86_avx512_maskz_vfmaddsub_pd_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movzbl %dil, %eax
; CHECK-NEXT:    kmovw %eax, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm3
; CHECK-NEXT:    vfmaddsub213pd %zmm2, %zmm1, %zmm3 {%k1} {z}
; CHECK-NEXT:    vfmaddsub213pd {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddpd %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <8 x double> @llvm.x86.avx512.maskz.vfmaddsub.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3, i32 4)
  %res1 = call <8 x double> @llvm.x86.avx512.maskz.vfmaddsub.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 -1, i32 0)
  %res2 = fadd <8 x double> %res, %res1
  ret <8 x double> %res2
}

define <16 x float>@test_int_x86_avx512_mask_vfmaddsub_ps_512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3){
; CHECK-LABEL: test_int_x86_avx512_mask_vfmaddsub_ps_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovw %edi, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm3
; CHECK-NEXT:    vfmaddsub213ps %zmm2, %zmm1, %zmm3 {%k1}
; CHECK-NEXT:    vfmaddsub213ps {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddps %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <16 x float> @llvm.x86.avx512.mask.vfmaddsub.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3, i32 4)
  %res1 = call <16 x float> @llvm.x86.avx512.mask.vfmaddsub.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 -1, i32 0)
  %res2 = fadd <16 x float> %res, %res1
  ret <16 x float> %res2
}

declare <16 x float> @llvm.x86.avx512.mask3.vfmaddsub.ps.512(<16 x float>, <16 x float>, <16 x float>, i16, i32)

define <16 x float>@test_int_x86_avx512_mask3_vfmaddsub_ps_512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3){
; CHECK-LABEL: test_int_x86_avx512_mask3_vfmaddsub_ps_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovw %edi, %k1
; CHECK-NEXT:    vmovaps %zmm2, %zmm3
; CHECK-NEXT:    vfmaddsub231ps %zmm1, %zmm0, %zmm3 {%k1}
; CHECK-NEXT:    vfmaddsub213ps {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddps %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <16 x float> @llvm.x86.avx512.mask3.vfmaddsub.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3, i32 4)
  %res1 = call <16 x float> @llvm.x86.avx512.mask3.vfmaddsub.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 -1, i32 0)
  %res2 = fadd <16 x float> %res, %res1
  ret <16 x float> %res2
}

declare <16 x float> @llvm.x86.avx512.maskz.vfmaddsub.ps.512(<16 x float>, <16 x float>, <16 x float>, i16, i32)

define <16 x float>@test_int_x86_avx512_maskz_vfmaddsub_ps_512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3){
; CHECK-LABEL: test_int_x86_avx512_maskz_vfmaddsub_ps_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovw %edi, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm3
; CHECK-NEXT:    vfmaddsub213ps %zmm2, %zmm1, %zmm3 {%k1} {z}
; CHECK-NEXT:    vfmaddsub213ps {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddps %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <16 x float> @llvm.x86.avx512.maskz.vfmaddsub.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3, i32 4)
  %res1 = call <16 x float> @llvm.x86.avx512.maskz.vfmaddsub.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 -1, i32 0)
  %res2 = fadd <16 x float> %res, %res1
  ret <16 x float> %res2
}

declare <8 x double> @llvm.x86.avx512.mask3.vfmsubadd.pd.512(<8 x double>, <8 x double>, <8 x double>, i8, i32)

define <8 x double>@test_int_x86_avx512_mask3_vfmsubadd_pd_512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3){
; CHECK-LABEL: test_int_x86_avx512_mask3_vfmsubadd_pd_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movzbl %dil, %eax
; CHECK-NEXT:    kmovw %eax, %k1
; CHECK-NEXT:    vmovaps %zmm2, %zmm3
; CHECK-NEXT:    vfmsubadd231pd %zmm1, %zmm0, %zmm3 {%k1}
; CHECK-NEXT:    vfmsubadd213pd {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddpd %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <8 x double> @llvm.x86.avx512.mask3.vfmsubadd.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3, i32 4)
  %res1 = call <8 x double> @llvm.x86.avx512.mask3.vfmsubadd.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 -1, i32 0)
  %res2 = fadd <8 x double> %res, %res1
  ret <8 x double> %res2
}

declare <16 x float> @llvm.x86.avx512.mask3.vfmsubadd.ps.512(<16 x float>, <16 x float>, <16 x float>, i16, i32)

define <16 x float>@test_int_x86_avx512_mask3_vfmsubadd_ps_512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3){
; CHECK-LABEL: test_int_x86_avx512_mask3_vfmsubadd_ps_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovw %edi, %k1
; CHECK-NEXT:    vmovaps %zmm2, %zmm3
; CHECK-NEXT:    vfmsubadd231ps %zmm1, %zmm0, %zmm3 {%k1}
; CHECK-NEXT:    vfmsubadd213ps {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddps %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <16 x float> @llvm.x86.avx512.mask3.vfmsubadd.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3, i32 4)
  %res1 = call <16 x float> @llvm.x86.avx512.mask3.vfmsubadd.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 -1, i32 0)
  %res2 = fadd <16 x float> %res, %res1
  ret <16 x float> %res2
}

define <16 x float> @test_mask_round_vfmadd512_ps_rrb_rne(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 %mask) {
  ; CHECK-LABEL: test_mask_round_vfmadd512_ps_rrb_rne
  ; CHECK: vfmadd213ps  {rn-sae}, %zmm2, %zmm1, %zmm0 {%k1} ## encoding: [0x62,0xf2,0x75,0x19,0xa8,0xc2]
  %res = call <16 x float> @llvm.x86.avx512.mask.vfmadd.ps.512(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 %mask, i32 0) nounwind
  ret <16 x float> %res
}

define <16 x float> @test_mask_round_vfmadd512_ps_rrb_rtn(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 %mask) {
  ; CHECK-LABEL: test_mask_round_vfmadd512_ps_rrb_rtn
  ; CHECK: vfmadd213ps  {rd-sae}, %zmm2, %zmm1, %zmm0 {%k1} ## encoding: [0x62,0xf2,0x75,0x39,0xa8,0xc2]
  %res = call <16 x float> @llvm.x86.avx512.mask.vfmadd.ps.512(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 %mask, i32 1) nounwind
  ret <16 x float> %res
}

define <16 x float> @test_mask_round_vfmadd512_ps_rrb_rtp(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 %mask) {
  ; CHECK-LABEL: test_mask_round_vfmadd512_ps_rrb_rtp
  ; CHECK: vfmadd213ps  {ru-sae}, %zmm2, %zmm1, %zmm0 {%k1} ## encoding: [0x62,0xf2,0x75,0x59,0xa8,0xc2]
  %res = call <16 x float> @llvm.x86.avx512.mask.vfmadd.ps.512(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 %mask, i32 2) nounwind
  ret <16 x float> %res
}

define <16 x float> @test_mask_round_vfmadd512_ps_rrb_rtz(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 %mask) {
  ; CHECK-LABEL: test_mask_round_vfmadd512_ps_rrb_rtz
  ; CHECK: vfmadd213ps  {rz-sae}, %zmm2, %zmm1, %zmm0 {%k1} ## encoding: [0x62,0xf2,0x75,0x79,0xa8,0xc2]
  %res = call <16 x float> @llvm.x86.avx512.mask.vfmadd.ps.512(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 %mask, i32 3) nounwind
  ret <16 x float> %res
}

define <16 x float> @test_mask_round_vfmadd512_ps_rrb_current(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 %mask) {
  ; CHECK-LABEL: test_mask_round_vfmadd512_ps_rrb_current
  ; CHECK: vfmadd213ps  %zmm2, %zmm1, %zmm0 {%k1} ## encoding: [0x62,0xf2,0x75,0x49,0xa8,0xc2]
  %res = call <16 x float> @llvm.x86.avx512.mask.vfmadd.ps.512(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 %mask, i32 4) nounwind
  ret <16 x float> %res
}

define <16 x float> @test_mask_round_vfmadd512_ps_rrbz_rne(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2) {
  ; CHECK-LABEL: test_mask_round_vfmadd512_ps_rrbz_rne
  ; CHECK: vfmadd213ps  {rn-sae}, %zmm2, %zmm1, %zmm0 ## encoding: [0x62,0xf2,0x75,0x18,0xa8,0xc2]
  %res = call <16 x float> @llvm.x86.avx512.mask.vfmadd.ps.512(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 -1, i32 0) nounwind
  ret <16 x float> %res
}

define <16 x float> @test_mask_round_vfmadd512_ps_rrbz_rtn(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2) {
  ; CHECK-LABEL: test_mask_round_vfmadd512_ps_rrbz_rtn
  ; CHECK: vfmadd213ps  {rd-sae}, %zmm2, %zmm1, %zmm0 ## encoding: [0x62,0xf2,0x75,0x38,0xa8,0xc2]
  %res = call <16 x float> @llvm.x86.avx512.mask.vfmadd.ps.512(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 -1, i32 1) nounwind
  ret <16 x float> %res
}

define <16 x float> @test_mask_round_vfmadd512_ps_rrbz_rtp(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2) {
  ; CHECK-LABEL: test_mask_round_vfmadd512_ps_rrbz_rtp
  ; CHECK: vfmadd213ps  {ru-sae}, %zmm2, %zmm1, %zmm0 ## encoding: [0x62,0xf2,0x75,0x58,0xa8,0xc2]
  %res = call <16 x float> @llvm.x86.avx512.mask.vfmadd.ps.512(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 -1, i32 2) nounwind
  ret <16 x float> %res
}

define <16 x float> @test_mask_round_vfmadd512_ps_rrbz_rtz(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2) {
  ; CHECK-LABEL: test_mask_round_vfmadd512_ps_rrbz_rtz
  ; CHECK: vfmadd213ps  {rz-sae}, %zmm2, %zmm1, %zmm0 ## encoding: [0x62,0xf2,0x75,0x78,0xa8,0xc2]
  %res = call <16 x float> @llvm.x86.avx512.mask.vfmadd.ps.512(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 -1, i32 3) nounwind
  ret <16 x float> %res
}

define <16 x float> @test_mask_round_vfmadd512_ps_rrbz_current(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2) {
  ; CHECK-LABEL: test_mask_round_vfmadd512_ps_rrbz_current
  ; CHECK: vfmadd213ps  %zmm2, %zmm1, %zmm0 ## encoding: [0x62,0xf2,0x75,0x48,0xa8,0xc2]
  %res = call <16 x float> @llvm.x86.avx512.mask.vfmadd.ps.512(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2, i16 -1, i32 4) nounwind
  ret <16 x float> %res
}

declare <8 x double> @llvm.x86.avx512.mask3.vfmsub.pd.512(<8 x double>, <8 x double>, <8 x double>, i8, i32)

define <8 x double>@test_int_x86_avx512_mask3_vfmsub_pd_512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3){
; CHECK-LABEL: test_int_x86_avx512_mask3_vfmsub_pd_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movzbl %dil, %eax
; CHECK-NEXT:    kmovw %eax, %k1
; CHECK-NEXT:    vmovaps %zmm2, %zmm3
; CHECK-NEXT:    vfmsub231pd %zmm1, %zmm0, %zmm3 {%k1}
; CHECK-NEXT:    vfmsub213pd {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddpd %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <8 x double> @llvm.x86.avx512.mask3.vfmsub.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3, i32 4)
  %res1 = call <8 x double> @llvm.x86.avx512.mask3.vfmsub.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 -1, i32 0)
  %res2 = fadd <8 x double> %res, %res1
  ret <8 x double> %res2
}

declare <16 x float> @llvm.x86.avx512.mask3.vfmsub.ps.512(<16 x float>, <16 x float>, <16 x float>, i16, i32)

define <16 x float>@test_int_x86_avx512_mask3_vfmsub_ps_512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3){
; CHECK-LABEL: test_int_x86_avx512_mask3_vfmsub_ps_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovw %edi, %k1
; CHECK-NEXT:    vmovaps %zmm2, %zmm3
; CHECK-NEXT:    vfmsub231ps %zmm1, %zmm0, %zmm3 {%k1}
; CHECK-NEXT:    vfmsub213ps {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddps %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <16 x float> @llvm.x86.avx512.mask3.vfmsub.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3, i32 4)
  %res1 = call <16 x float> @llvm.x86.avx512.mask3.vfmsub.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 -1, i32 0)
  %res2 = fadd <16 x float> %res, %res1
  ret <16 x float> %res2
}

define <8 x double> @test_mask_round_vfmadd512_pd_rrb_rne(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask) {
  ; CHECK-LABEL: test_mask_round_vfmadd512_pd_rrb_rne
  ; CHECK: vfmadd213pd  {rn-sae}, %zmm2, %zmm1, %zmm0 {%k1} ## encoding: [0x62,0xf2,0xf5,0x19,0xa8,0xc2]
  %res = call <8 x double> @llvm.x86.avx512.mask.vfmadd.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask, i32 0) nounwind
  ret <8 x double> %res
}

define <8 x double> @test_mask_round_vfmadd512_pd_rrb_rtn(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask) {
  ; CHECK-LABEL: test_mask_round_vfmadd512_pd_rrb_rtn
  ; CHECK: vfmadd213pd  {rd-sae}, %zmm2, %zmm1, %zmm0 {%k1} ## encoding: [0x62,0xf2,0xf5,0x39,0xa8,0xc2]
  %res = call <8 x double> @llvm.x86.avx512.mask.vfmadd.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask, i32 1) nounwind
  ret <8 x double> %res
}

define <8 x double> @test_mask_round_vfmadd512_pd_rrb_rtp(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask) {
  ; CHECK-LABEL: test_mask_round_vfmadd512_pd_rrb_rtp
  ; CHECK: vfmadd213pd  {ru-sae}, %zmm2, %zmm1, %zmm0 {%k1} ## encoding: [0x62,0xf2,0xf5,0x59,0xa8,0xc2]
  %res = call <8 x double> @llvm.x86.avx512.mask.vfmadd.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask, i32 2) nounwind
  ret <8 x double> %res
}

define <8 x double> @test_mask_round_vfmadd512_pd_rrb_rtz(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask) {
  ; CHECK-LABEL: test_mask_round_vfmadd512_pd_rrb_rtz
  ; CHECK: vfmadd213pd  {rz-sae}, %zmm2, %zmm1, %zmm0 {%k1} ## encoding: [0x62,0xf2,0xf5,0x79,0xa8,0xc2]
  %res = call <8 x double> @llvm.x86.avx512.mask.vfmadd.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask, i32 3) nounwind
  ret <8 x double> %res
}

define <8 x double> @test_mask_round_vfmadd512_pd_rrb_current(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask) {
  ; CHECK-LABEL: test_mask_round_vfmadd512_pd_rrb_current
  ; CHECK: vfmadd213pd  %zmm2, %zmm1, %zmm0 {%k1} ## encoding: [0x62,0xf2,0xf5,0x49,0xa8,0xc2]
  %res = call <8 x double> @llvm.x86.avx512.mask.vfmadd.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask, i32 4) nounwind
  ret <8 x double> %res
}

define <8 x double> @test_mask_round_vfmadd512_pd_rrbz_rne(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2) {
  ; CHECK-LABEL: test_mask_round_vfmadd512_pd_rrbz_rne
  ; CHECK: vfmadd213pd  {rn-sae}, %zmm2, %zmm1, %zmm0 ## encoding: [0x62,0xf2,0xf5,0x18,0xa8,0xc2]
  %res = call <8 x double> @llvm.x86.avx512.mask.vfmadd.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 -1, i32 0) nounwind
  ret <8 x double> %res
}

define <8 x double> @test_mask_round_vfmadd512_pd_rrbz_rtn(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2) {
  ; CHECK-LABEL: test_mask_round_vfmadd512_pd_rrbz_rtn
  ; CHECK: vfmadd213pd  {rd-sae}, %zmm2, %zmm1, %zmm0 ## encoding: [0x62,0xf2,0xf5,0x38,0xa8,0xc2]
  %res = call <8 x double> @llvm.x86.avx512.mask.vfmadd.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 -1, i32 1) nounwind
  ret <8 x double> %res
}

define <8 x double> @test_mask_round_vfmadd512_pd_rrbz_rtp(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2) {
  ; CHECK-LABEL: test_mask_round_vfmadd512_pd_rrbz_rtp
  ; CHECK: vfmadd213pd  {ru-sae}, %zmm2, %zmm1, %zmm0 ## encoding: [0x62,0xf2,0xf5,0x58,0xa8,0xc2]
  %res = call <8 x double> @llvm.x86.avx512.mask.vfmadd.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 -1, i32 2) nounwind
  ret <8 x double> %res
}

define <8 x double> @test_mask_round_vfmadd512_pd_rrbz_rtz(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2) {
  ; CHECK-LABEL: test_mask_round_vfmadd512_pd_rrbz_rtz
  ; CHECK: vfmadd213pd  {rz-sae}, %zmm2, %zmm1, %zmm0 ## encoding: [0x62,0xf2,0xf5,0x78,0xa8,0xc2]
  %res = call <8 x double> @llvm.x86.avx512.mask.vfmadd.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 -1, i32 3) nounwind
  ret <8 x double> %res
}

define <8 x double> @test_mask_round_vfmadd512_pd_rrbz_current(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2) {
  ; CHECK-LABEL: test_mask_round_vfmadd512_pd_rrbz_current
  ; CHECK: vfmadd213pd  %zmm2, %zmm1, %zmm0 ## encoding: [0x62,0xf2,0xf5,0x48,0xa8,0xc2]
  %res = call <8 x double> @llvm.x86.avx512.mask.vfmadd.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 -1, i32 4) nounwind
  ret <8 x double> %res
}

define <8 x double>@test_int_x86_avx512_mask_vfmadd_pd_512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3){
; CHECK-LABEL: test_int_x86_avx512_mask_vfmadd_pd_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movzbl %dil, %eax
; CHECK-NEXT:    kmovw %eax, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm3
; CHECK-NEXT:    vfmadd213pd %zmm2, %zmm1, %zmm3 {%k1}
; CHECK-NEXT:    vfmadd213pd {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddpd %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <8 x double> @llvm.x86.avx512.mask.vfmadd.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3, i32 4)
  %res1 = call <8 x double> @llvm.x86.avx512.mask.vfmadd.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 -1, i32 0)
  %res2 = fadd <8 x double> %res, %res1
  ret <8 x double> %res2
}

declare <8 x double> @llvm.x86.avx512.mask3.vfmadd.pd.512(<8 x double>, <8 x double>, <8 x double>, i8, i32)

define <8 x double>@test_int_x86_avx512_mask3_vfmadd_pd_512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3){
; CHECK-LABEL: test_int_x86_avx512_mask3_vfmadd_pd_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movzbl %dil, %eax
; CHECK-NEXT:    kmovw %eax, %k1
; CHECK-NEXT:    vmovaps %zmm2, %zmm3
; CHECK-NEXT:    vfmadd231pd %zmm1, %zmm0, %zmm3 {%k1}
; CHECK-NEXT:    vfmadd213pd {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddpd %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <8 x double> @llvm.x86.avx512.mask3.vfmadd.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3, i32 4)
  %res1 = call <8 x double> @llvm.x86.avx512.mask3.vfmadd.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 -1, i32 0)
  %res2 = fadd <8 x double> %res, %res1
  ret <8 x double> %res2
}

declare <8 x double> @llvm.x86.avx512.maskz.vfmadd.pd.512(<8 x double>, <8 x double>, <8 x double>, i8, i32)

define <8 x double>@test_int_x86_avx512_maskz_vfmadd_pd_512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3){
; CHECK-LABEL: test_int_x86_avx512_maskz_vfmadd_pd_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movzbl %dil, %eax
; CHECK-NEXT:    kmovw %eax, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm3
; CHECK-NEXT:    vfmadd213pd %zmm2, %zmm1, %zmm3 {%k1} {z}
; CHECK-NEXT:    vfmadd213pd {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddpd %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <8 x double> @llvm.x86.avx512.maskz.vfmadd.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3, i32 4)
  %res1 = call <8 x double> @llvm.x86.avx512.maskz.vfmadd.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 -1, i32 0)
  %res2 = fadd <8 x double> %res, %res1
  ret <8 x double> %res2
}

define <16 x float>@test_int_x86_avx512_mask_vfmadd_ps_512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3){
; CHECK-LABEL: test_int_x86_avx512_mask_vfmadd_ps_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovw %edi, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm3
; CHECK-NEXT:    vfmadd213ps %zmm2, %zmm1, %zmm3 {%k1}
; CHECK-NEXT:    vfmadd213ps {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddps %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <16 x float> @llvm.x86.avx512.mask.vfmadd.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3, i32 4)
  %res1 = call <16 x float> @llvm.x86.avx512.mask.vfmadd.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 -1, i32 0)
  %res2 = fadd <16 x float> %res, %res1
  ret <16 x float> %res2
}

declare <16 x float> @llvm.x86.avx512.mask3.vfmadd.ps.512(<16 x float>, <16 x float>, <16 x float>, i16, i32)

define <16 x float>@test_int_x86_avx512_mask3_vfmadd_ps_512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3){
; CHECK-LABEL: test_int_x86_avx512_mask3_vfmadd_ps_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovw %edi, %k1
; CHECK-NEXT:    vmovaps %zmm2, %zmm3
; CHECK-NEXT:    vfmadd231ps %zmm1, %zmm0, %zmm3 {%k1}
; CHECK-NEXT:    vfmadd213ps {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddps %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <16 x float> @llvm.x86.avx512.mask3.vfmadd.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3, i32 4)
  %res1 = call <16 x float> @llvm.x86.avx512.mask3.vfmadd.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 -1, i32 0)
  %res2 = fadd <16 x float> %res, %res1
  ret <16 x float> %res2
}

declare <16 x float> @llvm.x86.avx512.maskz.vfmadd.ps.512(<16 x float>, <16 x float>, <16 x float>, i16, i32)

define <16 x float>@test_int_x86_avx512_maskz_vfmadd_ps_512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3){
; CHECK-LABEL: test_int_x86_avx512_maskz_vfmadd_ps_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovw %edi, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm3
; CHECK-NEXT:    vfmadd213ps %zmm2, %zmm1, %zmm3 {%k1} {z}
; CHECK-NEXT:    vfmadd213ps {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddps %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <16 x float> @llvm.x86.avx512.maskz.vfmadd.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3, i32 4)
  %res1 = call <16 x float> @llvm.x86.avx512.maskz.vfmadd.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 -1, i32 0)
  %res2 = fadd <16 x float> %res, %res1
  ret <16 x float> %res2
}


define <8 x double> @test_mask_round_vfnmsub512_pd_rrb_rne(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask) {
  ; CHECK-LABEL: test_mask_round_vfnmsub512_pd_rrb_rne
  ; CHECK: vfnmsub213pd  {rn-sae}, %zmm2, %zmm1, %zmm0 {%k1} ## encoding: [0x62,0xf2,0xf5,0x19,0xae,0xc2]
  %res = call <8 x double> @llvm.x86.avx512.mask.vfnmsub.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask, i32 0) nounwind
  ret <8 x double> %res
}

define <8 x double> @test_mask_round_vfnmsub512_pd_rrb_rtn(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask) {
  ; CHECK-LABEL: test_mask_round_vfnmsub512_pd_rrb_rtn
  ; CHECK: vfnmsub213pd  {rd-sae}, %zmm2, %zmm1, %zmm0 {%k1} ## encoding: [0x62,0xf2,0xf5,0x39,0xae,0xc2]
  %res = call <8 x double> @llvm.x86.avx512.mask.vfnmsub.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask, i32 1) nounwind
  ret <8 x double> %res
}

define <8 x double> @test_mask_round_vfnmsub512_pd_rrb_rtp(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask) {
  ; CHECK-LABEL: test_mask_round_vfnmsub512_pd_rrb_rtp
  ; CHECK: vfnmsub213pd  {ru-sae}, %zmm2, %zmm1, %zmm0 {%k1} ## encoding: [0x62,0xf2,0xf5,0x59,0xae,0xc2]
  %res = call <8 x double> @llvm.x86.avx512.mask.vfnmsub.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask, i32 2) nounwind
  ret <8 x double> %res
}

define <8 x double> @test_mask_round_vfnmsub512_pd_rrb_rtz(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask) {
  ; CHECK-LABEL: test_mask_round_vfnmsub512_pd_rrb_rtz
  ; CHECK: vfnmsub213pd  {rz-sae}, %zmm2, %zmm1, %zmm0 {%k1} ## encoding: [0x62,0xf2,0xf5,0x79,0xae,0xc2]
  %res = call <8 x double> @llvm.x86.avx512.mask.vfnmsub.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask, i32 3) nounwind
  ret <8 x double> %res
}

define <8 x double> @test_mask_round_vfnmsub512_pd_rrb_current(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask) {
  ; CHECK-LABEL: test_mask_round_vfnmsub512_pd_rrb_current
  ; CHECK: vfnmsub213pd  %zmm2, %zmm1, %zmm0 {%k1} ## encoding: [0x62,0xf2,0xf5,0x49,0xae,0xc2]
  %res = call <8 x double> @llvm.x86.avx512.mask.vfnmsub.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 %mask, i32 4) nounwind
  ret <8 x double> %res
}

define <8 x double> @test_mask_round_vfnmsub512_pd_rrbz_rne(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2) {
  ; CHECK-LABEL: test_mask_round_vfnmsub512_pd_rrbz_rne
  ; CHECK: vfnmsub213pd  {rn-sae}, %zmm2, %zmm1, %zmm0 ## encoding: [0x62,0xf2,0xf5,0x18,0xae,0xc2]
  %res = call <8 x double> @llvm.x86.avx512.mask.vfnmsub.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 -1, i32 0) nounwind
  ret <8 x double> %res
}

define <8 x double> @test_mask_round_vfnmsub512_pd_rrbz_rtn(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2) {
  ; CHECK-LABEL: test_mask_round_vfnmsub512_pd_rrbz_rtn
  ; CHECK: vfnmsub213pd  {rd-sae}, %zmm2, %zmm1, %zmm0 ## encoding: [0x62,0xf2,0xf5,0x38,0xae,0xc2]
  %res = call <8 x double> @llvm.x86.avx512.mask.vfnmsub.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 -1, i32 1) nounwind
  ret <8 x double> %res
}

define <8 x double> @test_mask_round_vfnmsub512_pd_rrbz_rtp(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2) {
  ; CHECK-LABEL: test_mask_round_vfnmsub512_pd_rrbz_rtp
  ; CHECK: vfnmsub213pd  {ru-sae}, %zmm2, %zmm1, %zmm0 ## encoding: [0x62,0xf2,0xf5,0x58,0xae,0xc2]
  %res = call <8 x double> @llvm.x86.avx512.mask.vfnmsub.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 -1, i32 2) nounwind
  ret <8 x double> %res
}

define <8 x double> @test_mask_round_vfnmsub512_pd_rrbz_rtz(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2) {
  ; CHECK-LABEL: test_mask_round_vfnmsub512_pd_rrbz_rtz
  ; CHECK: vfnmsub213pd  {rz-sae}, %zmm2, %zmm1, %zmm0 ## encoding: [0x62,0xf2,0xf5,0x78,0xae,0xc2]
  %res = call <8 x double> @llvm.x86.avx512.mask.vfnmsub.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 -1, i32 3) nounwind
  ret <8 x double> %res
}

define <8 x double> @test_mask_round_vfnmsub512_pd_rrbz_current(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2) {
  ; CHECK-LABEL: test_mask_round_vfnmsub512_pd_rrbz_current
  ; CHECK: vfnmsub213pd  %zmm2, %zmm1, %zmm0 ## encoding: [0x62,0xf2,0xf5,0x48,0xae,0xc2]
  %res = call <8 x double> @llvm.x86.avx512.mask.vfnmsub.pd.512(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2, i8 -1, i32 4) nounwind
  ret <8 x double> %res
}

define <8 x double>@test_int_x86_avx512_mask_vfnmsub_pd_512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3){
; CHECK-LABEL: test_int_x86_avx512_mask_vfnmsub_pd_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movzbl %dil, %eax
; CHECK-NEXT:    kmovw %eax, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm3
; CHECK-NEXT:    vfnmsub213pd %zmm2, %zmm1, %zmm3 {%k1}
; CHECK-NEXT:    vfnmsub213pd {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddpd %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <8 x double> @llvm.x86.avx512.mask.vfnmsub.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3, i32 4)
  %res1 = call <8 x double> @llvm.x86.avx512.mask.vfnmsub.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 -1, i32 0)
  %res2 = fadd <8 x double> %res, %res1
  ret <8 x double> %res2
}

declare <8 x double> @llvm.x86.avx512.mask3.vfnmsub.pd.512(<8 x double>, <8 x double>, <8 x double>, i8, i32)

define <8 x double>@test_int_x86_avx512_mask3_vfnmsub_pd_512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3){
; CHECK-LABEL: test_int_x86_avx512_mask3_vfnmsub_pd_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movzbl %dil, %eax
; CHECK-NEXT:    kmovw %eax, %k1
; CHECK-NEXT:    vmovaps %zmm2, %zmm3
; CHECK-NEXT:    vfnmsub231pd %zmm1, %zmm0, %zmm3 {%k1}
; CHECK-NEXT:    vfnmsub213pd {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddpd %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <8 x double> @llvm.x86.avx512.mask3.vfnmsub.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3, i32 4)
  %res1 = call <8 x double> @llvm.x86.avx512.mask3.vfnmsub.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 -1, i32 0)
  %res2 = fadd <8 x double> %res, %res1
  ret <8 x double> %res2
}

define <16 x float>@test_int_x86_avx512_mask_vfnmsub_ps_512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3){
; CHECK-LABEL: test_int_x86_avx512_mask_vfnmsub_ps_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovw %edi, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm3
; CHECK-NEXT:    vfnmsub213ps %zmm2, %zmm1, %zmm3 {%k1}
; CHECK-NEXT:    vfnmsub213ps {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddps %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <16 x float> @llvm.x86.avx512.mask.vfnmsub.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3, i32 4)
  %res1 = call <16 x float> @llvm.x86.avx512.mask.vfnmsub.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 -1, i32 0)
  %res2 = fadd <16 x float> %res, %res1
  ret <16 x float> %res2
}

declare <16 x float> @llvm.x86.avx512.mask3.vfnmsub.ps.512(<16 x float>, <16 x float>, <16 x float>, i16, i32)

define <16 x float>@test_int_x86_avx512_mask3_vfnmsub_ps_512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3){
; CHECK-LABEL: test_int_x86_avx512_mask3_vfnmsub_ps_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovw %edi, %k1
; CHECK-NEXT:    vmovaps %zmm2, %zmm3
; CHECK-NEXT:    vfnmsub231ps %zmm1, %zmm0, %zmm3 {%k1}
; CHECK-NEXT:    vfnmsub213ps {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddps %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <16 x float> @llvm.x86.avx512.mask3.vfnmsub.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3, i32 4)
  %res1 = call <16 x float> @llvm.x86.avx512.mask3.vfnmsub.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 -1, i32 0)
  %res2 = fadd <16 x float> %res, %res1
  ret <16 x float> %res2
}

define <8 x double>@test_int_x86_avx512_mask_vfnmadd_pd_512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3){
; CHECK-LABEL: test_int_x86_avx512_mask_vfnmadd_pd_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movzbl %dil, %eax
; CHECK-NEXT:    kmovw %eax, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm3
; CHECK-NEXT:    vfnmadd213pd %zmm2, %zmm1, %zmm3 {%k1}
; CHECK-NEXT:    vfnmadd213pd {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddpd %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <8 x double> @llvm.x86.avx512.mask.vfnmadd.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 %x3, i32 4)
  %res1 = call <8 x double> @llvm.x86.avx512.mask.vfnmadd.pd.512(<8 x double> %x0, <8 x double> %x1, <8 x double> %x2, i8 -1, i32 0)
  %res2 = fadd <8 x double> %res, %res1
  ret <8 x double> %res2
}

define <16 x float>@test_int_x86_avx512_mask_vfnmadd_ps_512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3){
; CHECK-LABEL: test_int_x86_avx512_mask_vfnmadd_ps_512:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovw %edi, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm3
; CHECK-NEXT:    vfnmadd213ps %zmm2, %zmm1, %zmm3 {%k1}
; CHECK-NEXT:    vfnmadd213ps {rn-sae}, %zmm2, %zmm1, %zmm0
; CHECK-NEXT:    vaddps %zmm0, %zmm3, %zmm0
; CHECK-NEXT:    retq
  %res = call <16 x float> @llvm.x86.avx512.mask.vfnmadd.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 %x3, i32 4)
  %res1 = call <16 x float> @llvm.x86.avx512.mask.vfnmadd.ps.512(<16 x float> %x0, <16 x float> %x1, <16 x float> %x2, i16 -1, i32 0)
  %res2 = fadd <16 x float> %res, %res1
  ret <16 x float> %res2
}

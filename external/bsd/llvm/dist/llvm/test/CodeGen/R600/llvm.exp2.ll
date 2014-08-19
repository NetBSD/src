;RUN: llc < %s -march=r600 -mcpu=redwood | FileCheck %s --check-prefix=EG-CHECK --check-prefix=FUNC
;RUN: llc < %s -march=r600 -mcpu=cayman | FileCheck %s --check-prefix=CM-CHECK --check-prefix=FUNC
;RUN: llc < %s -march=r600 -mcpu=SI | FileCheck %s --check-prefix=SI-CHECK --check-prefix=FUNC

;FUNC-LABEL: @test
;EG-CHECK: EXP_IEEE
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}} (MASKED)
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}} (MASKED)
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}} (MASKED)
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}}
;SI-CHECK: V_EXP_F32

define void @test(float addrspace(1)* %out, float %in) {
entry:
   %0 = call float @llvm.exp2.f32(float %in)
   store float %0, float addrspace(1)* %out
   ret void
}

;FUNC-LABEL: @testv2
;EG-CHECK: EXP_IEEE
;EG-CHECK: EXP_IEEE
; FIXME: We should be able to merge these packets together on Cayman so we
; have a maximum of 4 instructions.
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}} (MASKED)
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}} (MASKED)
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}} (MASKED)
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}} (MASKED)
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}} (MASKED)
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}} (MASKED)
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}}
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}}
;SI-CHECK: V_EXP_F32
;SI-CHECK: V_EXP_F32

define void @testv2(<2 x float> addrspace(1)* %out, <2 x float> %in) {
entry:
  %0 = call <2 x float> @llvm.exp2.v2f32(<2 x float> %in)
  store <2 x float> %0, <2 x float> addrspace(1)* %out
  ret void
}

;FUNC-LABEL: @testv4
;EG-CHECK: EXP_IEEE
;EG-CHECK: EXP_IEEE
;EG-CHECK: EXP_IEEE
;EG-CHECK: EXP_IEEE
; FIXME: We should be able to merge these packets together on Cayman so we
; have a maximum of 4 instructions.
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}} (MASKED)
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}} (MASKED)
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}} (MASKED)
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}} (MASKED)
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}} (MASKED)
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}} (MASKED)
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}} (MASKED)
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}} (MASKED)
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}} (MASKED)
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}} (MASKED)
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}} (MASKED)
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}} (MASKED)
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}}
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}}
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}}
;CM-CHECK-DAG: EXP_IEEE T{{[0-9]+\.[XYZW]}}
;SI-CHECK: V_EXP_F32
;SI-CHECK: V_EXP_F32
;SI-CHECK: V_EXP_F32
;SI-CHECK: V_EXP_F32
define void @testv4(<4 x float> addrspace(1)* %out, <4 x float> %in) {
entry:
  %0 = call <4 x float> @llvm.exp2.v4f32(<4 x float> %in)
  store <4 x float> %0, <4 x float> addrspace(1)* %out
  ret void
}

declare float @llvm.exp2.f32(float) readnone
declare <2 x float> @llvm.exp2.v2f32(<2 x float>) readnone
declare <4 x float> @llvm.exp2.v4f32(<4 x float>) readnone

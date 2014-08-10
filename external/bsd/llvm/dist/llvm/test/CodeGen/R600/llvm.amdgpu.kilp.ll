; RUN: llc -march=r600 -mcpu=verde -verify-machineinstrs < %s | FileCheck -check-prefix=SI %s

; SI-LABEL: @kilp_gs_const
; SI: S_MOV_B64 exec, 0
define void @kilp_gs_const() #0 {
main_body:
  %0 = icmp ule i32 0, 3
  %1 = select i1 %0, float 1.000000e+00, float -1.000000e+00
  call void @llvm.AMDGPU.kilp(float %1)
  %2 = icmp ule i32 3, 0
  %3 = select i1 %2, float 1.000000e+00, float -1.000000e+00
  call void @llvm.AMDGPU.kilp(float %3)
  ret void
}

declare void @llvm.AMDGPU.kilp(float)

attributes #0 = { "ShaderType"="2" }

!0 = metadata !{metadata !"const", null, i32 1}

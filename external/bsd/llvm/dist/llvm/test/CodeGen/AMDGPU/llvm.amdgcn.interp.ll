;RUN: llc < %s -march=amdgcn -mcpu=verde -verify-machineinstrs | FileCheck --check-prefix=GCN %s
;RUN: llc < %s -march=amdgcn -mcpu=tonga -verify-machineinstrs | FileCheck --check-prefixes=GCN,VI %s

; GCN-LABEL: {{^}}v_interp:
; GCN-NOT: s_wqm
; GCN: s_mov_b32 m0, s{{[0-9]+}}
; GCN-DAG: v_interp_p1_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr0.x{{$}}
; GCN-DAG: v_interp_p1_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr0.y{{$}}
; GCN-DAG: v_interp_p2_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr0.y{{$}}
; GCN-DAG: v_interp_mov_f32 v{{[0-9]+}}, p0, attr0.x{{$}}
define amdgpu_ps void @v_interp(<16 x i8> addrspace(2)* inreg, <16 x i8> addrspace(2)* inreg, <32 x i8> addrspace(2)* inreg, i32 inreg, <2 x float>) {
main_body:
  %i = extractelement <2 x float> %4, i32 0
  %j = extractelement <2 x float> %4, i32 1
  %p0_0 = call float @llvm.amdgcn.interp.p1(float %i, i32 0, i32 0, i32 %3)
  %p1_0 = call float @llvm.amdgcn.interp.p2(float %p0_0, float %j, i32 0, i32 0, i32 %3)
  %p0_1 = call float @llvm.amdgcn.interp.p1(float %i, i32 1, i32 0, i32 %3)
  %p1_1 = call float @llvm.amdgcn.interp.p2(float %p0_1, float %j, i32 1, i32 0, i32 %3)
  %const = call float @llvm.amdgcn.interp.mov(i32 2, i32 0, i32 0, i32 %3)
  %w = fadd float %p1_1, %const
  call void @llvm.SI.export(i32 15, i32 1, i32 1, i32 0, i32 1, float %p0_0, float %p0_0, float %p1_1, float %w)
  ret void
}

; GCN-LABEL: {{^}}v_interp_p1:
; GCN: s_movk_i32 m0, 0x100
; GCN-DAG: v_interp_p1_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr0.x{{$}}
; GCN-DAG: v_interp_p1_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr0.y{{$}}
; GCN-DAG: v_interp_p1_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr0.z{{$}}
; GCN-DAG: v_interp_p1_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr0.w{{$}}
; GCN-DAG: v_interp_p1_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr0.x{{$}}

; GCN-DAG: v_interp_p1_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr1.x{{$}}
; GCN-DAG: v_interp_p1_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr2.y{{$}}
; GCN-DAG: v_interp_p1_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr3.z{{$}}
; GCN-DAG: v_interp_p1_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr4.w{{$}}
; GCN-DAG: v_interp_p1_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr63.w{{$}}
; GCN-DAG: v_interp_p1_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr64.w{{$}}
; GCN-DAG: v_interp_p1_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr64.x{{$}}
define amdgpu_ps void @v_interp_p1(float %i) {
  %p0_0 = call float @llvm.amdgcn.interp.p1(float %i, i32 0, i32 0, i32 256)
  %p0_1 = call float @llvm.amdgcn.interp.p1(float %i, i32 1, i32 0, i32 256)
  %p0_2 = call float @llvm.amdgcn.interp.p1(float %i, i32 2, i32 0, i32 256)
  %p0_3 = call float @llvm.amdgcn.interp.p1(float %i, i32 3, i32 0, i32 256)
  %p0_4 = call float @llvm.amdgcn.interp.p1(float %i, i32 4, i32 0, i32 256)
  %p0_5 = call float @llvm.amdgcn.interp.p1(float %i, i32 0, i32 1, i32 256)
  %p0_6 = call float @llvm.amdgcn.interp.p1(float %i, i32 1, i32 2, i32 256)
  %p0_7 = call float @llvm.amdgcn.interp.p1(float %i, i32 2, i32 3, i32 256)
  %p0_8 = call float @llvm.amdgcn.interp.p1(float %i, i32 3, i32 4, i32 256)
  %p0_9 = call float @llvm.amdgcn.interp.p1(float %i, i32 3, i32 63, i32 256)
  %p0_10 = call float @llvm.amdgcn.interp.p1(float %i, i32 3, i32 64, i32 256)
  %p0_11 = call float @llvm.amdgcn.interp.p1(float %i, i32 4, i32 64, i32 256)

  store volatile float %p0_0, float addrspace(1)* undef
  store volatile float %p0_1, float addrspace(1)* undef
  store volatile float %p0_2, float addrspace(1)* undef
  store volatile float %p0_3, float addrspace(1)* undef
  store volatile float %p0_4, float addrspace(1)* undef
  store volatile float %p0_5, float addrspace(1)* undef
  store volatile float %p0_6, float addrspace(1)* undef
  store volatile float %p0_7, float addrspace(1)* undef
  store volatile float %p0_8, float addrspace(1)* undef
  store volatile float %p0_9, float addrspace(1)* undef
  store volatile float %p0_10, float addrspace(1)* undef
  store volatile float %p0_11, float addrspace(1)* undef
  ret void
}

; GCN-LABEL: {{^}}v_interp_p2:
; GCN: s_movk_i32 m0, 0x100
; GCN-DAG: v_interp_p2_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr0.x{{$}}
; GCN-DAG: v_interp_p2_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr0.y{{$}}
; GCN-DAG: v_interp_p2_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr0.z{{$}}
; GCN-DAG: v_interp_p2_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr0.w{{$}}
; GCN-DAG: v_interp_p2_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr0.x{{$}}
; GCN-DAG: v_interp_p2_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr0.x{{$}}
; GCN-DAG: v_interp_p2_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr63.x{{$}}
; GCN-DAG: v_interp_p2_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr64.x{{$}}
; GCN-DAG: v_interp_p2_f32 v{{[0-9]+}}, v{{[0-9]+}}, attr64.x{{$}}
define amdgpu_ps void @v_interp_p2(float %x, float %j) {
  %p2_0 = call float @llvm.amdgcn.interp.p2(float %x, float %j, i32 0, i32 0, i32 256)
  %p2_1 = call float @llvm.amdgcn.interp.p2(float %x, float %j, i32 1, i32 0, i32 256)
  %p2_2 = call float @llvm.amdgcn.interp.p2(float %x, float %j, i32 2, i32 0, i32 256)
  %p2_3 = call float @llvm.amdgcn.interp.p2(float %x, float %j, i32 3, i32 0, i32 256)
  %p2_4 = call float @llvm.amdgcn.interp.p2(float %x, float %j, i32 4, i32 0, i32 256)

  %p2_5 = call float @llvm.amdgcn.interp.p2(float %x, float %j, i32 0, i32 1, i32 256)
  %p2_6 = call float @llvm.amdgcn.interp.p2(float %x, float %j, i32 0, i32 63, i32 256)
  %p2_7 = call float @llvm.amdgcn.interp.p2(float %x, float %j, i32 0, i32 64, i32 256)
  %p2_8 = call float @llvm.amdgcn.interp.p2(float %x, float %j, i32 4, i32 64, i32 256)

  store volatile float %p2_0, float addrspace(1)* undef
  store volatile float %p2_1, float addrspace(1)* undef
  store volatile float %p2_2, float addrspace(1)* undef
  store volatile float %p2_3, float addrspace(1)* undef
  store volatile float %p2_4, float addrspace(1)* undef
  store volatile float %p2_5, float addrspace(1)* undef
  store volatile float %p2_6, float addrspace(1)* undef
  store volatile float %p2_7, float addrspace(1)* undef
  store volatile float %p2_8, float addrspace(1)* undef
  ret void
}

; GCN-LABEL: {{^}}v_interp_mov:
; GCN: s_movk_i32 m0, 0x100
; GCN-DAG: v_interp_mov_f32 v{{[0-9]+}}, p10, attr0.x{{$}}
; GCN-DAG: v_interp_mov_f32 v{{[0-9]+}}, p20, attr0.x{{$}}
; GCN-DAG: v_interp_mov_f32 v{{[0-9]+}}, p0, attr0.x{{$}}
; GCN-DAG: v_interp_mov_f32 v{{[0-9]+}}, invalid_param_3, attr0.x{{$}}

; GCN-DAG: v_interp_mov_f32 v{{[0-9]+}}, p10, attr0.x{{$}}
; GCN-DAG: v_interp_mov_f32 v{{[0-9]+}}, p10, attr0.z{{$}}
; GCN-DAG: v_interp_mov_f32 v{{[0-9]+}}, p10, attr0.w{{$}}
; GCN-DAG: v_interp_mov_f32 v{{[0-9]+}}, p10, attr0.x{{$}}
; GCN-DAG: v_interp_mov_f32 v{{[0-9]+}}, invalid_param_8, attr0.x{{$}}

; GCN-DAG: v_interp_mov_f32 v{{[0-9]+}}, p10, attr63.y{{$}}
; GCN-DAG: v_interp_mov_f32 v{{[0-9]+}}, p10, attr64.y{{$}}
; GCN-DAG: v_interp_mov_f32 v{{[0-9]+}}, invalid_param_3, attr64.y{{$}}
; GCN-DAG: v_interp_mov_f32 v{{[0-9]+}}, invalid_param_10, attr64.x{{$}}
define amdgpu_ps void @v_interp_mov(float %x, float %j) {
  %mov_0 = call float @llvm.amdgcn.interp.mov(i32 0, i32 0, i32 0, i32 256)
  %mov_1 = call float @llvm.amdgcn.interp.mov(i32 1, i32 0, i32 0, i32 256)
  %mov_2 = call float @llvm.amdgcn.interp.mov(i32 2, i32 0, i32 0, i32 256)
  %mov_3 = call float @llvm.amdgcn.interp.mov(i32 3, i32 0, i32 0, i32 256)

  %mov_4 = call float @llvm.amdgcn.interp.mov(i32 0, i32 1, i32 0, i32 256)
  %mov_5 = call float @llvm.amdgcn.interp.mov(i32 0, i32 2, i32 0, i32 256)
  %mov_6 = call float @llvm.amdgcn.interp.mov(i32 0, i32 3, i32 0, i32 256)
  %mov_7 = call float @llvm.amdgcn.interp.mov(i32 0, i32 4, i32 0, i32 256)
  %mov_8 = call float @llvm.amdgcn.interp.mov(i32 8, i32 4, i32 0, i32 256)

  %mov_9 = call float @llvm.amdgcn.interp.mov(i32 0, i32 1, i32 63, i32 256)
  %mov_10 = call float @llvm.amdgcn.interp.mov(i32 0, i32 1, i32 64, i32 256)
  %mov_11 = call float @llvm.amdgcn.interp.mov(i32 3, i32 1, i32 64, i32 256)
  %mov_12 = call float @llvm.amdgcn.interp.mov(i32 10, i32 4, i32 64, i32 256)

  store volatile float %mov_0, float addrspace(1)* undef
  store volatile float %mov_1, float addrspace(1)* undef
  store volatile float %mov_2, float addrspace(1)* undef
  store volatile float %mov_3, float addrspace(1)* undef

  store volatile float %mov_4, float addrspace(1)* undef
  store volatile float %mov_5, float addrspace(1)* undef
  store volatile float %mov_6, float addrspace(1)* undef
  store volatile float %mov_7, float addrspace(1)* undef
  store volatile float %mov_8, float addrspace(1)* undef

  store volatile float %mov_9, float addrspace(1)* undef
  store volatile float %mov_10, float addrspace(1)* undef
  store volatile float %mov_11, float addrspace(1)* undef
  store volatile float %mov_12, float addrspace(1)* undef
  ret void
}

; SI won't merge ds memory operations, because of the signed offset bug, so
; we only have check lines for VI.
; VI-LABEL: v_interp_readnone:
; VI: s_mov_b32 m0, 0
; VI-DAG: v_mov_b32_e32 [[ZERO:v[0-9]+]], 0
; VI-DAG: v_interp_mov_f32 v{{[0-9]+}}, p0, attr0.x{{$}}
; VI: s_mov_b32 m0, -1{{$}}
; VI: ds_write2_b32 v{{[0-9]+}}, [[ZERO]], [[ZERO]] offset1:4
define amdgpu_ps void @v_interp_readnone(float addrspace(3)* %lds) {
  store float 0.0, float addrspace(3)* %lds
  %tmp1 = call float @llvm.amdgcn.interp.mov(i32 2, i32 0, i32 0, i32 0)
  %tmp2 = getelementptr float, float addrspace(3)* %lds, i32 4
  store float 0.0, float addrspace(3)* %tmp2
  call void @llvm.SI.export(i32 15, i32 1, i32 1, i32 0, i32 1, float %tmp1, float %tmp1, float %tmp1, float %tmp1)
  ret void
}

; Function Attrs: nounwind readnone
declare float @llvm.amdgcn.interp.p1(float, i32, i32, i32) #0

; Function Attrs: nounwind readnone
declare float @llvm.amdgcn.interp.p2(float, float, i32, i32, i32) #0

declare float @llvm.amdgcn.interp.mov(i32, i32, i32, i32) #0

declare void @llvm.SI.export(i32, i32, i32, i32, i32, float, float, float, float)

attributes #0 = { nounwind readnone }

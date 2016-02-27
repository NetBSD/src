; RUN: llc -march=amdgcn -mcpu=SI -verify-machineinstrs < %s | FileCheck -check-prefix=SI -check-prefix=FUNC %s
; RUN: llc -march=amdgcn -mcpu=tonga -verify-machineinstrs < %s | FileCheck -check-prefix=SI -check-prefix=FUNC %s
; RUN: llc -march=r600 -mcpu=cypress -verify-machineinstrs < %s | FileCheck -check-prefix=EG -check-prefix=FUNC %s

declare i8 @llvm.ctlz.i8(i8, i1) nounwind readnone

declare i32 @llvm.ctlz.i32(i32, i1) nounwind readnone
declare <2 x i32> @llvm.ctlz.v2i32(<2 x i32>, i1) nounwind readnone
declare <4 x i32> @llvm.ctlz.v4i32(<4 x i32>, i1) nounwind readnone

declare i64 @llvm.ctlz.i64(i64, i1) nounwind readnone
declare <2 x i64> @llvm.ctlz.v2i64(<2 x i64>, i1) nounwind readnone
declare <4 x i64> @llvm.ctlz.v4i64(<4 x i64>, i1) nounwind readnone

declare i32 @llvm.r600.read.tidig.x() nounwind readnone

; FUNC-LABEL: {{^}}s_ctlz_zero_undef_i32:
; SI: s_load_dword [[VAL:s[0-9]+]],
; SI: s_flbit_i32_b32 [[SRESULT:s[0-9]+]], [[VAL]]
; SI: v_mov_b32_e32 [[VRESULT:v[0-9]+]], [[SRESULT]]
; SI: buffer_store_dword [[VRESULT]],
; SI: s_endpgm
; EG: MEM_RAT_CACHELESS STORE_RAW [[RESULT:T[0-9]+\.[XYZW]]]
; EG: FFBH_UINT {{\*? *}}[[RESULT]]
define void @s_ctlz_zero_undef_i32(i32 addrspace(1)* noalias %out, i32 %val) nounwind {
  %ctlz = call i32 @llvm.ctlz.i32(i32 %val, i1 true) nounwind readnone
  store i32 %ctlz, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: {{^}}v_ctlz_zero_undef_i32:
; SI: buffer_load_dword [[VAL:v[0-9]+]],
; SI: v_ffbh_u32_e32 [[RESULT:v[0-9]+]], [[VAL]]
; SI: buffer_store_dword [[RESULT]],
; SI: s_endpgm
; EG: MEM_RAT_CACHELESS STORE_RAW [[RESULT:T[0-9]+\.[XYZW]]]
; EG: FFBH_UINT {{\*? *}}[[RESULT]]
define void @v_ctlz_zero_undef_i32(i32 addrspace(1)* noalias %out, i32 addrspace(1)* noalias %valptr) nounwind {
  %val = load i32, i32 addrspace(1)* %valptr, align 4
  %ctlz = call i32 @llvm.ctlz.i32(i32 %val, i1 true) nounwind readnone
  store i32 %ctlz, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: {{^}}v_ctlz_zero_undef_v2i32:
; SI: buffer_load_dwordx2
; SI: v_ffbh_u32_e32
; SI: v_ffbh_u32_e32
; SI: buffer_store_dwordx2
; SI: s_endpgm
; EG: MEM_RAT_CACHELESS STORE_RAW [[RESULT:T[0-9]+]]{{\.[XYZW]}}
; EG: FFBH_UINT {{\*? *}}[[RESULT]]
; EG: FFBH_UINT {{\*? *}}[[RESULT]]
define void @v_ctlz_zero_undef_v2i32(<2 x i32> addrspace(1)* noalias %out, <2 x i32> addrspace(1)* noalias %valptr) nounwind {
  %val = load <2 x i32>, <2 x i32> addrspace(1)* %valptr, align 8
  %ctlz = call <2 x i32> @llvm.ctlz.v2i32(<2 x i32> %val, i1 true) nounwind readnone
  store <2 x i32> %ctlz, <2 x i32> addrspace(1)* %out, align 8
  ret void
}

; FUNC-LABEL: {{^}}v_ctlz_zero_undef_v4i32:
; SI: buffer_load_dwordx4
; SI: v_ffbh_u32_e32
; SI: v_ffbh_u32_e32
; SI: v_ffbh_u32_e32
; SI: v_ffbh_u32_e32
; SI: buffer_store_dwordx4
; SI: s_endpgm
; EG: MEM_RAT_CACHELESS STORE_RAW [[RESULT:T[0-9]+]]{{\.[XYZW]}}
; EG: FFBH_UINT {{\*? *}}[[RESULT]]
; EG: FFBH_UINT {{\*? *}}[[RESULT]]
; EG: FFBH_UINT {{\*? *}}[[RESULT]]
; EG: FFBH_UINT {{\*? *}}[[RESULT]]
define void @v_ctlz_zero_undef_v4i32(<4 x i32> addrspace(1)* noalias %out, <4 x i32> addrspace(1)* noalias %valptr) nounwind {
  %val = load <4 x i32>, <4 x i32> addrspace(1)* %valptr, align 16
  %ctlz = call <4 x i32> @llvm.ctlz.v4i32(<4 x i32> %val, i1 true) nounwind readnone
  store <4 x i32> %ctlz, <4 x i32> addrspace(1)* %out, align 16
  ret void
}

; FUNC-LABEL: {{^}}v_ctlz_zero_undef_i8:
; SI: buffer_load_ubyte [[VAL:v[0-9]+]],
; SI: v_ffbh_u32_e32 [[FFBH:v[0-9]+]], [[VAL]]
; SI: v_add_i32_e32 [[RESULT:v[0-9]+]], vcc, 0xffffffe8, [[FFBH]]
; SI: buffer_store_byte [[RESULT]],
define void @v_ctlz_zero_undef_i8(i8 addrspace(1)* noalias %out, i8 addrspace(1)* noalias %valptr) nounwind {
  %val = load i8, i8 addrspace(1)* %valptr
  %ctlz = call i8 @llvm.ctlz.i8(i8 %val, i1 true) nounwind readnone
  store i8 %ctlz, i8 addrspace(1)* %out
  ret void
}

; FUNC-LABEL: {{^}}s_ctlz_zero_undef_i64:
; SI: s_load_dwordx2 s{{\[}}[[LO:[0-9]+]]:[[HI:[0-9]+]]{{\]}}, s{{\[[0-9]+:[0-9]+\]}}, {{0xb|0x2c}}
; SI-DAG: v_cmp_eq_i32_e64 vcc, 0, s[[HI]]
; SI-DAG: s_flbit_i32_b32 [[FFBH_LO:s[0-9]+]], s[[LO]]
; SI-DAG: s_add_i32 [[ADD:s[0-9]+]], [[FFBH_LO]], 32
; SI-DAG: s_flbit_i32_b32 [[FFBH_HI:s[0-9]+]], s[[HI]]
; SI-DAG: v_mov_b32_e32 [[VFFBH_LO:v[0-9]+]], [[FFBH_LO]]
; SI-DAG: v_mov_b32_e32 [[VFFBH_HI:v[0-9]+]], [[FFBH_HI]]
; SI-DAG: v_cndmask_b32_e32 v[[CTLZ:[0-9]+]], [[VFFBH_HI]], [[VFFBH_LO]]
; SI-DAG: v_mov_b32_e32 v[[CTLZ_HI:[0-9]+]], 0{{$}}
; SI: {{buffer|flat}}_store_dwordx2 v{{\[}}[[CTLZ]]:[[CTLZ_HI]]{{\]}}
define void @s_ctlz_zero_undef_i64(i64 addrspace(1)* noalias %out, i64 %val) nounwind {
  %ctlz = call i64 @llvm.ctlz.i64(i64 %val, i1 true)
  store i64 %ctlz, i64 addrspace(1)* %out
  ret void
}

; FUNC-LABEL: {{^}}s_ctlz_zero_undef_i64_trunc:
define void @s_ctlz_zero_undef_i64_trunc(i32 addrspace(1)* noalias %out, i64 %val) nounwind {
  %ctlz = call i64 @llvm.ctlz.i64(i64 %val, i1 true)
  %trunc = trunc i64 %ctlz to i32
  store i32 %trunc, i32 addrspace(1)* %out
  ret void
}

; FUNC-LABEL: {{^}}v_ctlz_zero_undef_i64:
; SI: {{buffer|flat}}_load_dwordx2 v{{\[}}[[LO:[0-9]+]]:[[HI:[0-9]+]]{{\]}}
; SI-DAG: v_cmp_eq_i32_e64 [[CMPHI:s\[[0-9]+:[0-9]+\]]], 0, v[[HI]]
; SI-DAG: v_ffbh_u32_e32 [[FFBH_LO:v[0-9]+]], v[[LO]]
; SI-DAG: v_add_i32_e32 [[ADD:v[0-9]+]], vcc, 32, [[FFBH_LO]]
; SI-DAG: v_ffbh_u32_e32 [[FFBH_HI:v[0-9]+]], v[[HI]]
; SI-DAG: v_cndmask_b32_e64 v[[CTLZ:[0-9]+]], [[FFBH_HI]], [[FFBH_LO]]
; SI-DAG: v_mov_b32_e32 v[[CTLZ_HI:[0-9]+]], 0{{$}}
; SI: {{buffer|flat}}_store_dwordx2 v{{\[}}[[CTLZ]]:[[CTLZ_HI]]{{\]}}
define void @v_ctlz_zero_undef_i64(i64 addrspace(1)* noalias %out, i64 addrspace(1)* noalias %in) nounwind {
  %tid = call i32 @llvm.r600.read.tidig.x()
  %in.gep = getelementptr i64, i64 addrspace(1)* %in, i32 %tid
  %out.gep = getelementptr i64, i64 addrspace(1)* %out, i32 %tid
  %val = load i64, i64 addrspace(1)* %in.gep
  %ctlz = call i64 @llvm.ctlz.i64(i64 %val, i1 true)
  store i64 %ctlz, i64 addrspace(1)* %out.gep
  ret void
}

; FUNC-LABEL: {{^}}v_ctlz_zero_undef_i64_trunc:
define void @v_ctlz_zero_undef_i64_trunc(i32 addrspace(1)* noalias %out, i64 addrspace(1)* noalias %in) nounwind {
  %tid = call i32 @llvm.r600.read.tidig.x()
  %in.gep = getelementptr i64, i64 addrspace(1)* %in, i32 %tid
  %out.gep = getelementptr i32, i32 addrspace(1)* %out, i32 %tid
  %val = load i64, i64 addrspace(1)* %in.gep
  %ctlz = call i64 @llvm.ctlz.i64(i64 %val, i1 true)
  %trunc = trunc i64 %ctlz to i32
  store i32 %trunc, i32 addrspace(1)* %out.gep
  ret void
}

; FUNC-LABEL: {{^}}v_ctlz_zero_undef_i32_sel_eq_neg1:
; SI: buffer_load_dword [[VAL:v[0-9]+]],
; SI: v_ffbh_u32_e32 [[RESULT:v[0-9]+]], [[VAL]]
; SI-NEXT: buffer_store_dword [[RESULT]],
 define void @v_ctlz_zero_undef_i32_sel_eq_neg1(i32 addrspace(1)* noalias %out, i32 addrspace(1)* noalias %valptr) nounwind {
  %val = load i32, i32 addrspace(1)* %valptr
  %ctlz = call i32 @llvm.ctlz.i32(i32 %val, i1 true) nounwind readnone
  %cmp = icmp eq i32 %val, 0
  %sel = select i1 %cmp, i32 -1, i32 %ctlz
  store i32 %sel, i32 addrspace(1)* %out
  ret void
}

; FUNC-LABEL: {{^}}v_ctlz_zero_undef_i32_sel_ne_neg1:
; SI: buffer_load_dword [[VAL:v[0-9]+]],
; SI: v_ffbh_u32_e32 [[RESULT:v[0-9]+]], [[VAL]]
; SI-NEXT: buffer_store_dword [[RESULT]],
define void @v_ctlz_zero_undef_i32_sel_ne_neg1(i32 addrspace(1)* noalias %out, i32 addrspace(1)* noalias %valptr) nounwind {
  %val = load i32, i32 addrspace(1)* %valptr
  %ctlz = call i32 @llvm.ctlz.i32(i32 %val, i1 true) nounwind readnone
  %cmp = icmp ne i32 %val, 0
  %sel = select i1 %cmp, i32 %ctlz, i32 -1
  store i32 %sel, i32 addrspace(1)* %out
  ret void
}

; FUNC-LABEL: {{^}}v_ctlz_zero_undef_i8_sel_eq_neg1:
; SI: buffer_load_ubyte [[VAL:v[0-9]+]],
; SI: v_ffbh_u32_e32 [[FFBH:v[0-9]+]], [[VAL]]
; SI: buffer_store_byte [[FFBH]],
 define void @v_ctlz_zero_undef_i8_sel_eq_neg1(i8 addrspace(1)* noalias %out, i8 addrspace(1)* noalias %valptr) nounwind {
  %val = load i8, i8 addrspace(1)* %valptr
  %ctlz = call i8 @llvm.ctlz.i8(i8 %val, i1 true) nounwind readnone
  %cmp = icmp eq i8 %val, 0
  %sel = select i1 %cmp, i8 -1, i8 %ctlz
  store i8 %sel, i8 addrspace(1)* %out
  ret void
}

; FUNC-LABEL: {{^}}v_ctlz_zero_undef_i32_sel_eq_neg1_two_use:
; SI: buffer_load_dword [[VAL:v[0-9]+]],
; SI-DAG: v_ffbh_u32_e32 [[RESULT0:v[0-9]+]], [[VAL]]
; SI-DAG: v_cmp_eq_i32_e32 vcc, 0, [[VAL]]
; SI-DAG: v_cndmask_b32_e64 [[RESULT1:v[0-9]+]], 0, 1, vcc
; SI-DAG: buffer_store_dword [[RESULT0]]
; SI-DAG: buffer_store_byte [[RESULT1]]
; SI: s_endpgm
 define void @v_ctlz_zero_undef_i32_sel_eq_neg1_two_use(i32 addrspace(1)* noalias %out, i32 addrspace(1)* noalias %valptr) nounwind {
  %val = load i32, i32 addrspace(1)* %valptr
  %ctlz = call i32 @llvm.ctlz.i32(i32 %val, i1 true) nounwind readnone
  %cmp = icmp eq i32 %val, 0
  %sel = select i1 %cmp, i32 -1, i32 %ctlz
  store volatile i32 %sel, i32 addrspace(1)* %out
  store volatile i1 %cmp, i1 addrspace(1)* undef
  ret void
}

; Selected on wrong constant
; FUNC-LABEL: {{^}}v_ctlz_zero_undef_i32_sel_eq_0:
; SI: buffer_load_dword
; SI: v_ffbh_u32_e32
; SI: v_cmp
; SI: v_cndmask
; SI: buffer_store_dword
 define void @v_ctlz_zero_undef_i32_sel_eq_0(i32 addrspace(1)* noalias %out, i32 addrspace(1)* noalias %valptr) nounwind {
  %val = load i32, i32 addrspace(1)* %valptr
  %ctlz = call i32 @llvm.ctlz.i32(i32 %val, i1 true) nounwind readnone
  %cmp = icmp eq i32 %val, 0
  %sel = select i1 %cmp, i32 0, i32 %ctlz
  store i32 %sel, i32 addrspace(1)* %out
  ret void
}

; Selected on wrong constant
; FUNC-LABEL: {{^}}v_ctlz_zero_undef_i32_sel_ne_0:
; SI: buffer_load_dword
; SI: v_ffbh_u32_e32
; SI: v_cmp
; SI: v_cndmask
; SI: buffer_store_dword
define void @v_ctlz_zero_undef_i32_sel_ne_0(i32 addrspace(1)* noalias %out, i32 addrspace(1)* noalias %valptr) nounwind {
  %val = load i32, i32 addrspace(1)* %valptr
  %ctlz = call i32 @llvm.ctlz.i32(i32 %val, i1 true) nounwind readnone
  %cmp = icmp ne i32 %val, 0
  %sel = select i1 %cmp, i32 %ctlz, i32 0
  store i32 %sel, i32 addrspace(1)* %out
  ret void
}

; Compare on wrong constant
; FUNC-LABEL: {{^}}v_ctlz_zero_undef_i32_sel_eq_cmp_non0:
; SI: buffer_load_dword
; SI: v_ffbh_u32_e32
; SI: v_cmp
; SI: v_cndmask
; SI: buffer_store_dword
 define void @v_ctlz_zero_undef_i32_sel_eq_cmp_non0(i32 addrspace(1)* noalias %out, i32 addrspace(1)* noalias %valptr) nounwind {
  %val = load i32, i32 addrspace(1)* %valptr
  %ctlz = call i32 @llvm.ctlz.i32(i32 %val, i1 true) nounwind readnone
  %cmp = icmp eq i32 %val, 1
  %sel = select i1 %cmp, i32 0, i32 %ctlz
  store i32 %sel, i32 addrspace(1)* %out
  ret void
}

; Selected on wrong constant
; FUNC-LABEL: {{^}}v_ctlz_zero_undef_i32_sel_ne_cmp_non0:
; SI: buffer_load_dword
; SI: v_ffbh_u32_e32
; SI: v_cmp
; SI: v_cndmask
; SI: buffer_store_dword
define void @v_ctlz_zero_undef_i32_sel_ne_cmp_non0(i32 addrspace(1)* noalias %out, i32 addrspace(1)* noalias %valptr) nounwind {
  %val = load i32, i32 addrspace(1)* %valptr
  %ctlz = call i32 @llvm.ctlz.i32(i32 %val, i1 true) nounwind readnone
  %cmp = icmp ne i32 %val, 1
  %sel = select i1 %cmp, i32 %ctlz, i32 0
  store i32 %sel, i32 addrspace(1)* %out
  ret void
}

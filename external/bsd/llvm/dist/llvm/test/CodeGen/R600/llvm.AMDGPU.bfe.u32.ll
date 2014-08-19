; RUN: llc -march=r600 -mcpu=SI -verify-machineinstrs < %s | FileCheck -check-prefix=SI -check-prefix=FUNC %s
; RUN: llc -march=r600 -mcpu=redwood -verify-machineinstrs < %s | FileCheck -check-prefix=EG -check-prefix=FUNC %s

declare i32 @llvm.AMDGPU.bfe.u32(i32, i32, i32) nounwind readnone

; FUNC-LABEL: @bfe_u32_arg_arg_arg
; SI: V_BFE_U32
; EG: BFE_UINT
define void @bfe_u32_arg_arg_arg(i32 addrspace(1)* %out, i32 %src0, i32 %src1, i32 %src2) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 %src0, i32 %src1, i32 %src1) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_arg_arg_imm
; SI: V_BFE_U32
; EG: BFE_UINT
define void @bfe_u32_arg_arg_imm(i32 addrspace(1)* %out, i32 %src0, i32 %src1) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 %src0, i32 %src1, i32 123) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_arg_imm_arg
; SI: V_BFE_U32
; EG: BFE_UINT
define void @bfe_u32_arg_imm_arg(i32 addrspace(1)* %out, i32 %src0, i32 %src2) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 %src0, i32 123, i32 %src2) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_imm_arg_arg
; SI: V_BFE_U32
; EG: BFE_UINT
define void @bfe_u32_imm_arg_arg(i32 addrspace(1)* %out, i32 %src1, i32 %src2) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 123, i32 %src1, i32 %src2) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_arg_0_width_reg_offset
; SI-NOT: BFE
; SI: S_ENDPGM
; EG-NOT: BFE
define void @bfe_u32_arg_0_width_reg_offset(i32 addrspace(1)* %out, i32 %src0, i32 %src1) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 %src0, i32 %src1, i32 0) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_arg_0_width_imm_offset
; SI-NOT: BFE
; SI: S_ENDPGM
; EG-NOT: BFE
define void @bfe_u32_arg_0_width_imm_offset(i32 addrspace(1)* %out, i32 %src0, i32 %src1) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 %src0, i32 8, i32 0) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_zextload_i8
; SI: BUFFER_LOAD_UBYTE
; SI-NOT: BFE
; SI: S_ENDPGM
define void @bfe_u32_zextload_i8(i32 addrspace(1)* %out, i8 addrspace(1)* %in) nounwind {
  %load = load i8 addrspace(1)* %in
  %ext = zext i8 %load to i32
  %bfe = call i32 @llvm.AMDGPU.bfe.u32(i32 %ext, i32 0, i32 8)
  store i32 %bfe, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_zext_in_reg_i8
; SI: BUFFER_LOAD_DWORD
; SI: V_ADD_I32
; SI-NEXT: V_AND_B32_e32
; SI-NOT: BFE
; SI: S_ENDPGM
define void @bfe_u32_zext_in_reg_i8(i32 addrspace(1)* %out, i32 addrspace(1)* %in) nounwind {
  %load = load i32 addrspace(1)* %in, align 4
  %add = add i32 %load, 1
  %ext = and i32 %add, 255
  %bfe = call i32 @llvm.AMDGPU.bfe.u32(i32 %ext, i32 0, i32 8)
  store i32 %bfe, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_zext_in_reg_i16
; SI: BUFFER_LOAD_DWORD
; SI: V_ADD_I32
; SI-NEXT: V_AND_B32_e32
; SI-NOT: BFE
; SI: S_ENDPGM
define void @bfe_u32_zext_in_reg_i16(i32 addrspace(1)* %out, i32 addrspace(1)* %in) nounwind {
  %load = load i32 addrspace(1)* %in, align 4
  %add = add i32 %load, 1
  %ext = and i32 %add, 65535
  %bfe = call i32 @llvm.AMDGPU.bfe.u32(i32 %ext, i32 0, i32 16)
  store i32 %bfe, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_zext_in_reg_i8_offset_1
; SI: BUFFER_LOAD_DWORD
; SI: V_ADD_I32
; SI: BFE
; SI: S_ENDPGM
define void @bfe_u32_zext_in_reg_i8_offset_1(i32 addrspace(1)* %out, i32 addrspace(1)* %in) nounwind {
  %load = load i32 addrspace(1)* %in, align 4
  %add = add i32 %load, 1
  %ext = and i32 %add, 255
  %bfe = call i32 @llvm.AMDGPU.bfe.u32(i32 %ext, i32 1, i32 8)
  store i32 %bfe, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_zext_in_reg_i8_offset_3
; SI: BUFFER_LOAD_DWORD
; SI: V_ADD_I32
; SI-NEXT: V_AND_B32_e32 {{v[0-9]+}}, 0xf8
; SI-NEXT: BFE
; SI: S_ENDPGM
define void @bfe_u32_zext_in_reg_i8_offset_3(i32 addrspace(1)* %out, i32 addrspace(1)* %in) nounwind {
  %load = load i32 addrspace(1)* %in, align 4
  %add = add i32 %load, 1
  %ext = and i32 %add, 255
  %bfe = call i32 @llvm.AMDGPU.bfe.u32(i32 %ext, i32 3, i32 8)
  store i32 %bfe, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_zext_in_reg_i8_offset_7
; SI: BUFFER_LOAD_DWORD
; SI: V_ADD_I32
; SI-NEXT: V_AND_B32_e32 {{v[0-9]+}}, 0x80
; SI-NEXT: BFE
; SI: S_ENDPGM
define void @bfe_u32_zext_in_reg_i8_offset_7(i32 addrspace(1)* %out, i32 addrspace(1)* %in) nounwind {
  %load = load i32 addrspace(1)* %in, align 4
  %add = add i32 %load, 1
  %ext = and i32 %add, 255
  %bfe = call i32 @llvm.AMDGPU.bfe.u32(i32 %ext, i32 7, i32 8)
  store i32 %bfe, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_zext_in_reg_i16_offset_8
; SI: BUFFER_LOAD_DWORD
; SI: V_ADD_I32
; SI-NEXT: BFE
; SI: S_ENDPGM
define void @bfe_u32_zext_in_reg_i16_offset_8(i32 addrspace(1)* %out, i32 addrspace(1)* %in) nounwind {
  %load = load i32 addrspace(1)* %in, align 4
  %add = add i32 %load, 1
  %ext = and i32 %add, 65535
  %bfe = call i32 @llvm.AMDGPU.bfe.u32(i32 %ext, i32 8, i32 8)
  store i32 %bfe, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_test_1
; SI: BUFFER_LOAD_DWORD
; SI: V_AND_B32_e32 {{v[0-9]+}}, 1, {{v[0-9]+}}
; SI: S_ENDPGM
; EG: AND_INT T{{[0-9]\.[XYZW]}}, T{{[0-9]\.[XYZW]}}, 1,
define void @bfe_u32_test_1(i32 addrspace(1)* %out, i32 addrspace(1)* %in) nounwind {
  %x = load i32 addrspace(1)* %in, align 4
  %bfe = call i32 @llvm.AMDGPU.bfe.u32(i32 %x, i32 0, i32 1)
  store i32 %bfe, i32 addrspace(1)* %out, align 4
  ret void
}

define void @bfe_u32_test_2(i32 addrspace(1)* %out, i32 addrspace(1)* %in) nounwind {
  %x = load i32 addrspace(1)* %in, align 4
  %shl = shl i32 %x, 31
  %bfe = call i32 @llvm.AMDGPU.bfe.u32(i32 %shl, i32 0, i32 8)
  store i32 %bfe, i32 addrspace(1)* %out, align 4
  ret void
}

define void @bfe_u32_test_3(i32 addrspace(1)* %out, i32 addrspace(1)* %in) nounwind {
  %x = load i32 addrspace(1)* %in, align 4
  %shl = shl i32 %x, 31
  %bfe = call i32 @llvm.AMDGPU.bfe.u32(i32 %shl, i32 0, i32 1)
  store i32 %bfe, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_test_4
; SI-NOT: LSHL
; SI-NOT: SHR
; SI-NOT: BFE
; SI: V_MOV_B32_e32 [[VREG:v[0-9]+]], 0
; SI: BUFFER_STORE_DWORD [[VREG]],
; SI: S_ENDPGM
define void @bfe_u32_test_4(i32 addrspace(1)* %out, i32 addrspace(1)* %in) nounwind {
  %x = load i32 addrspace(1)* %in, align 4
  %shl = shl i32 %x, 31
  %shr = lshr i32 %shl, 31
  %bfe = call i32 @llvm.AMDGPU.bfe.u32(i32 %shr, i32 31, i32 1)
  store i32 %bfe, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_test_5
; SI: BUFFER_LOAD_DWORD
; SI-NOT: LSHL
; SI-NOT: SHR
; SI: V_BFE_I32 {{v[0-9]+}}, {{v[0-9]+}}, 0, 1
; SI: S_ENDPGM
define void @bfe_u32_test_5(i32 addrspace(1)* %out, i32 addrspace(1)* %in) nounwind {
  %x = load i32 addrspace(1)* %in, align 4
  %shl = shl i32 %x, 31
  %shr = ashr i32 %shl, 31
  %bfe = call i32 @llvm.AMDGPU.bfe.u32(i32 %shr, i32 0, i32 1)
  store i32 %bfe, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_test_6
; SI: V_LSHLREV_B32_e32 v{{[0-9]+}}, 31, v{{[0-9]+}}
; SI: V_LSHRREV_B32_e32 v{{[0-9]+}}, 1, v{{[0-9]+}}
; SI: S_ENDPGM
define void @bfe_u32_test_6(i32 addrspace(1)* %out, i32 addrspace(1)* %in) nounwind {
  %x = load i32 addrspace(1)* %in, align 4
  %shl = shl i32 %x, 31
  %bfe = call i32 @llvm.AMDGPU.bfe.u32(i32 %shl, i32 1, i32 31)
  store i32 %bfe, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_test_7
; SI: V_LSHLREV_B32_e32 v{{[0-9]+}}, 31, v{{[0-9]+}}
; SI-NOT: BFE
; SI: S_ENDPGM
define void @bfe_u32_test_7(i32 addrspace(1)* %out, i32 addrspace(1)* %in) nounwind {
  %x = load i32 addrspace(1)* %in, align 4
  %shl = shl i32 %x, 31
  %bfe = call i32 @llvm.AMDGPU.bfe.u32(i32 %shl, i32 0, i32 31)
  store i32 %bfe, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_test_8
; SI-NOT: BFE
; SI: V_AND_B32_e32 {{v[0-9]+}}, 1, {{v[0-9]+}}
; SI-NOT: BFE
; SI: S_ENDPGM
define void @bfe_u32_test_8(i32 addrspace(1)* %out, i32 addrspace(1)* %in) nounwind {
  %x = load i32 addrspace(1)* %in, align 4
  %shl = shl i32 %x, 31
  %bfe = call i32 @llvm.AMDGPU.bfe.u32(i32 %shl, i32 31, i32 1)
  store i32 %bfe, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_test_9
; SI-NOT: BFE
; SI: V_LSHRREV_B32_e32 v{{[0-9]+}}, 31, v{{[0-9]+}}
; SI-NOT: BFE
; SI: S_ENDPGM
define void @bfe_u32_test_9(i32 addrspace(1)* %out, i32 addrspace(1)* %in) nounwind {
  %x = load i32 addrspace(1)* %in, align 4
  %bfe = call i32 @llvm.AMDGPU.bfe.u32(i32 %x, i32 31, i32 1)
  store i32 %bfe, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_test_10
; SI-NOT: BFE
; SI: V_LSHRREV_B32_e32 v{{[0-9]+}}, 1, v{{[0-9]+}}
; SI-NOT: BFE
; SI: S_ENDPGM
define void @bfe_u32_test_10(i32 addrspace(1)* %out, i32 addrspace(1)* %in) nounwind {
  %x = load i32 addrspace(1)* %in, align 4
  %bfe = call i32 @llvm.AMDGPU.bfe.u32(i32 %x, i32 1, i32 31)
  store i32 %bfe, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_test_11
; SI-NOT: BFE
; SI: V_LSHRREV_B32_e32 v{{[0-9]+}}, 8, v{{[0-9]+}}
; SI-NOT: BFE
; SI: S_ENDPGM
define void @bfe_u32_test_11(i32 addrspace(1)* %out, i32 addrspace(1)* %in) nounwind {
  %x = load i32 addrspace(1)* %in, align 4
  %bfe = call i32 @llvm.AMDGPU.bfe.u32(i32 %x, i32 8, i32 24)
  store i32 %bfe, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_test_12
; SI-NOT: BFE
; SI: V_LSHRREV_B32_e32 v{{[0-9]+}}, 24, v{{[0-9]+}}
; SI-NOT: BFE
; SI: S_ENDPGM
define void @bfe_u32_test_12(i32 addrspace(1)* %out, i32 addrspace(1)* %in) nounwind {
  %x = load i32 addrspace(1)* %in, align 4
  %bfe = call i32 @llvm.AMDGPU.bfe.u32(i32 %x, i32 24, i32 8)
  store i32 %bfe, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_test_13
; V_ASHRREV_U32_e32 {{v[0-9]+}}, 31, {{v[0-9]+}}
; SI-NOT: BFE
; SI: S_ENDPGM
define void @bfe_u32_test_13(i32 addrspace(1)* %out, i32 addrspace(1)* %in) nounwind {
  %x = load i32 addrspace(1)* %in, align 4
  %shl = ashr i32 %x, 31
  %bfe = call i32 @llvm.AMDGPU.bfe.u32(i32 %shl, i32 31, i32 1)
  store i32 %bfe, i32 addrspace(1)* %out, align 4 ret void
}

; FUNC-LABEL: @bfe_u32_test_14
; SI-NOT: LSHR
; SI-NOT: BFE
; SI: S_ENDPGM
define void @bfe_u32_test_14(i32 addrspace(1)* %out, i32 addrspace(1)* %in) nounwind {
  %x = load i32 addrspace(1)* %in, align 4
  %shl = lshr i32 %x, 31
  %bfe = call i32 @llvm.AMDGPU.bfe.u32(i32 %shl, i32 31, i32 1)
  store i32 %bfe, i32 addrspace(1)* %out, align 4 ret void
}

; FUNC-LABEL: @bfe_u32_constant_fold_test_0
; SI-NOT: BFE
; SI: V_MOV_B32_e32 [[VREG:v[0-9]+]], 0
; SI: BUFFER_STORE_DWORD [[VREG]],
; SI: S_ENDPGM
; EG-NOT: BFE
define void @bfe_u32_constant_fold_test_0(i32 addrspace(1)* %out) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 0, i32 0, i32 0) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_constant_fold_test_1
; SI-NOT: BFE
; SI: V_MOV_B32_e32 [[VREG:v[0-9]+]], 0
; SI: BUFFER_STORE_DWORD [[VREG]],
; SI: S_ENDPGM
; EG-NOT: BFE
define void @bfe_u32_constant_fold_test_1(i32 addrspace(1)* %out) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 12334, i32 0, i32 0) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_constant_fold_test_2
; SI-NOT: BFE
; SI: V_MOV_B32_e32 [[VREG:v[0-9]+]], 0
; SI: BUFFER_STORE_DWORD [[VREG]],
; SI: S_ENDPGM
; EG-NOT: BFE
define void @bfe_u32_constant_fold_test_2(i32 addrspace(1)* %out) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 0, i32 0, i32 1) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_constant_fold_test_3
; SI-NOT: BFE
; SI: V_MOV_B32_e32 [[VREG:v[0-9]+]], 1
; SI: BUFFER_STORE_DWORD [[VREG]],
; SI: S_ENDPGM
; EG-NOT: BFE
define void @bfe_u32_constant_fold_test_3(i32 addrspace(1)* %out) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 1, i32 0, i32 1) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_constant_fold_test_4
; SI-NOT: BFE
; SI: V_MOV_B32_e32 [[VREG:v[0-9]+]], -1
; SI: BUFFER_STORE_DWORD [[VREG]],
; SI: S_ENDPGM
; EG-NOT: BFE
define void @bfe_u32_constant_fold_test_4(i32 addrspace(1)* %out) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 4294967295, i32 0, i32 1) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_constant_fold_test_5
; SI-NOT: BFE
; SI: V_MOV_B32_e32 [[VREG:v[0-9]+]], 1
; SI: BUFFER_STORE_DWORD [[VREG]],
; SI: S_ENDPGM
; EG-NOT: BFE
define void @bfe_u32_constant_fold_test_5(i32 addrspace(1)* %out) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 128, i32 7, i32 1) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_constant_fold_test_6
; SI-NOT: BFE
; SI: V_MOV_B32_e32 [[VREG:v[0-9]+]], 0x80
; SI: BUFFER_STORE_DWORD [[VREG]],
; SI: S_ENDPGM
; EG-NOT: BFE
define void @bfe_u32_constant_fold_test_6(i32 addrspace(1)* %out) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 128, i32 0, i32 8) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_constant_fold_test_7
; SI-NOT: BFE
; SI: V_MOV_B32_e32 [[VREG:v[0-9]+]], 0x7f
; SI: BUFFER_STORE_DWORD [[VREG]],
; SI: S_ENDPGM
; EG-NOT: BFE
define void @bfe_u32_constant_fold_test_7(i32 addrspace(1)* %out) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 127, i32 0, i32 8) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_constant_fold_test_8
; SI-NOT: BFE
; SI: V_MOV_B32_e32 [[VREG:v[0-9]+]], 1
; SI: BUFFER_STORE_DWORD [[VREG]],
; SI: S_ENDPGM
; EG-NOT: BFE
define void @bfe_u32_constant_fold_test_8(i32 addrspace(1)* %out) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 127, i32 6, i32 8) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_constant_fold_test_9
; SI-NOT: BFE
; SI: V_MOV_B32_e32 [[VREG:v[0-9]+]], 1
; SI: BUFFER_STORE_DWORD [[VREG]],
; SI: S_ENDPGM
; EG-NOT: BFEfppppppppppppp
define void @bfe_u32_constant_fold_test_9(i32 addrspace(1)* %out) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 65536, i32 16, i32 8) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_constant_fold_test_10
; SI-NOT: BFE
; SI: V_MOV_B32_e32 [[VREG:v[0-9]+]], 0
; SI: BUFFER_STORE_DWORD [[VREG]],
; SI: S_ENDPGM
; EG-NOT: BFE
define void @bfe_u32_constant_fold_test_10(i32 addrspace(1)* %out) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 65535, i32 16, i32 16) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_constant_fold_test_11
; SI-NOT: BFE
; SI: V_MOV_B32_e32 [[VREG:v[0-9]+]], 10
; SI: BUFFER_STORE_DWORD [[VREG]],
; SI: S_ENDPGM
; EG-NOT: BFE
define void @bfe_u32_constant_fold_test_11(i32 addrspace(1)* %out) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 160, i32 4, i32 4) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_constant_fold_test_12
; SI-NOT: BFE
; SI: V_MOV_B32_e32 [[VREG:v[0-9]+]], 0
; SI: BUFFER_STORE_DWORD [[VREG]],
; SI: S_ENDPGM
; EG-NOT: BFE
define void @bfe_u32_constant_fold_test_12(i32 addrspace(1)* %out) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 160, i32 31, i32 1) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_constant_fold_test_13
; SI-NOT: BFE
; SI: V_MOV_B32_e32 [[VREG:v[0-9]+]], 1
; SI: BUFFER_STORE_DWORD [[VREG]],
; SI: S_ENDPGM
; EG-NOT: BFE
define void @bfe_u32_constant_fold_test_13(i32 addrspace(1)* %out) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 131070, i32 16, i32 16) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_constant_fold_test_14
; SI-NOT: BFE
; SI: V_MOV_B32_e32 [[VREG:v[0-9]+]], 40
; SI: BUFFER_STORE_DWORD [[VREG]],
; SI: S_ENDPGM
; EG-NOT: BFE
define void @bfe_u32_constant_fold_test_14(i32 addrspace(1)* %out) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 160, i32 2, i32 30) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_constant_fold_test_15
; SI-NOT: BFE
; SI: V_MOV_B32_e32 [[VREG:v[0-9]+]], 10
; SI: BUFFER_STORE_DWORD [[VREG]],
; SI: S_ENDPGM
; EG-NOT: BFE
define void @bfe_u32_constant_fold_test_15(i32 addrspace(1)* %out) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 160, i32 4, i32 28) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_constant_fold_test_16
; SI-NOT: BFE
; SI: V_MOV_B32_e32 [[VREG:v[0-9]+]], 0x7f
; SI: BUFFER_STORE_DWORD [[VREG]],
; SI: S_ENDPGM
; EG-NOT: BFE
define void @bfe_u32_constant_fold_test_16(i32 addrspace(1)* %out) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 4294967295, i32 1, i32 7) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_constant_fold_test_17
; SI-NOT: BFE
; SI: V_MOV_B32_e32 [[VREG:v[0-9]+]], 0x7f
; SI: BUFFER_STORE_DWORD [[VREG]],
; SI: S_ENDPGM
; EG-NOT: BFE
define void @bfe_u32_constant_fold_test_17(i32 addrspace(1)* %out) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 255, i32 1, i32 31) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: @bfe_u32_constant_fold_test_18
; SI-NOT: BFE
; SI: V_MOV_B32_e32 [[VREG:v[0-9]+]], 0
; SI: BUFFER_STORE_DWORD [[VREG]],
; SI: S_ENDPGM
; EG-NOT: BFE
define void @bfe_u32_constant_fold_test_18(i32 addrspace(1)* %out) nounwind {
  %bfe_u32 = call i32 @llvm.AMDGPU.bfe.u32(i32 255, i32 31, i32 1) nounwind readnone
  store i32 %bfe_u32, i32 addrspace(1)* %out, align 4
  ret void
}

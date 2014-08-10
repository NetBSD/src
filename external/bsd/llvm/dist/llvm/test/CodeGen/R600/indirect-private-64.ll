; RUN: llc -march=r600 -mcpu=SI -verify-machineinstrs < %s | FileCheck -check-prefix=SI %s

declare void @llvm.AMDGPU.barrier.local() noduplicate nounwind

; SI-LABEL: @private_access_f64_alloca:
; SI: V_MOVRELD_B32_e32
; SI: V_MOVRELD_B32_e32
; SI: V_MOVRELS_B32_e32
; SI: V_MOVRELS_B32_e32
define void @private_access_f64_alloca(double addrspace(1)* noalias %out, double addrspace(1)* noalias %in, i32 %b) nounwind {
  %val = load double addrspace(1)* %in, align 8
  %array = alloca double, i32 16, align 8
  %ptr = getelementptr double* %array, i32 %b
  store double %val, double* %ptr, align 8
  call void @llvm.AMDGPU.barrier.local() noduplicate nounwind
  %result = load double* %ptr, align 8
  store double %result, double addrspace(1)* %out, align 8
  ret void
}

; SI-LABEL: @private_access_v2f64_alloca:
; SI: V_MOVRELD_B32_e32
; SI: V_MOVRELD_B32_e32
; SI: V_MOVRELD_B32_e32
; SI: V_MOVRELD_B32_e32
; SI: V_MOVRELS_B32_e32
; SI: V_MOVRELS_B32_e32
; SI: V_MOVRELS_B32_e32
; SI: V_MOVRELS_B32_e32
define void @private_access_v2f64_alloca(<2 x double> addrspace(1)* noalias %out, <2 x double> addrspace(1)* noalias %in, i32 %b) nounwind {
  %val = load <2 x double> addrspace(1)* %in, align 16
  %array = alloca <2 x double>, i32 16, align 16
  %ptr = getelementptr <2 x double>* %array, i32 %b
  store <2 x double> %val, <2 x double>* %ptr, align 16
  call void @llvm.AMDGPU.barrier.local() noduplicate nounwind
  %result = load <2 x double>* %ptr, align 16
  store <2 x double> %result, <2 x double> addrspace(1)* %out, align 16
  ret void
}

; SI-LABEL: @private_access_i64_alloca:
; SI: V_MOVRELD_B32_e32
; SI: V_MOVRELD_B32_e32
; SI: V_MOVRELS_B32_e32
; SI: V_MOVRELS_B32_e32
define void @private_access_i64_alloca(i64 addrspace(1)* noalias %out, i64 addrspace(1)* noalias %in, i32 %b) nounwind {
  %val = load i64 addrspace(1)* %in, align 8
  %array = alloca i64, i32 16, align 8
  %ptr = getelementptr i64* %array, i32 %b
  store i64 %val, i64* %ptr, align 8
  call void @llvm.AMDGPU.barrier.local() noduplicate nounwind
  %result = load i64* %ptr, align 8
  store i64 %result, i64 addrspace(1)* %out, align 8
  ret void
}

; SI-LABEL: @private_access_v2i64_alloca:
; SI: V_MOVRELD_B32_e32
; SI: V_MOVRELD_B32_e32
; SI: V_MOVRELD_B32_e32
; SI: V_MOVRELD_B32_e32
; SI: V_MOVRELS_B32_e32
; SI: V_MOVRELS_B32_e32
; SI: V_MOVRELS_B32_e32
; SI: V_MOVRELS_B32_e32
define void @private_access_v2i64_alloca(<2 x i64> addrspace(1)* noalias %out, <2 x i64> addrspace(1)* noalias %in, i32 %b) nounwind {
  %val = load <2 x i64> addrspace(1)* %in, align 16
  %array = alloca <2 x i64>, i32 16, align 16
  %ptr = getelementptr <2 x i64>* %array, i32 %b
  store <2 x i64> %val, <2 x i64>* %ptr, align 16
  call void @llvm.AMDGPU.barrier.local() noduplicate nounwind
  %result = load <2 x i64>* %ptr, align 16
  store <2 x i64> %result, <2 x i64> addrspace(1)* %out, align 16
  ret void
}

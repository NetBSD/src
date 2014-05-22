; RUN: llc < %s -march=r600 -mcpu=redwood | FileCheck --check-prefix=EG-CHECK %s
; RUN: llc < %s -march=r600 -mcpu=cayman | FileCheck --check-prefix=CM-CHECK %s
; RUN: llc < %s -march=r600 -mcpu=verde -verify-machineinstrs | FileCheck --check-prefix=SI-CHECK %s

;===------------------------------------------------------------------------===;
; Global Address Space
;===------------------------------------------------------------------------===;

; i8 store
; EG-CHECK-LABEL: @store_i8
; EG-CHECK: MEM_RAT MSKOR T[[RW_GPR:[0-9]]].XW, T{{[0-9]}}.X
; EG-CHECK: VTX_READ_8 [[VAL:T[0-9]\.X]], [[VAL]]
; IG 0: Get the byte index and truncate the value
; EG-CHECK: AND_INT T{{[0-9]}}.[[BI_CHAN:[XYZW]]], KC0[2].Y, literal.x
; EG-CHECK-NEXT: AND_INT * T{{[0-9]}}.[[TRUNC_CHAN:[XYZW]]], [[VAL]], literal.y
; EG-CHECK-NEXT: 3(4.203895e-45), 255(3.573311e-43)
; IG 1: Truncate the calculated the shift amount for the mask
; EG-CHECK: LSHL * T{{[0-9]}}.[[SHIFT_CHAN:[XYZW]]], PV.[[BI_CHAN]], literal.x
; EG-CHECK-NEXT: 3
; IG 2: Shift the value and the mask
; EG-CHECK: LSHL T[[RW_GPR]].X, T{{[0-9]}}.[[TRUNC_CHAN]], PV.[[SHIFT_CHAN]]
; EG-CHECK: LSHL * T[[RW_GPR]].W, literal.x, PV.[[SHIFT_CHAN]]
; EG-CHECK-NEXT: 255
; IG 3: Initialize the Y and Z channels to zero
;       XXX: An optimal scheduler should merge this into one of the prevous IGs.
; EG-CHECK: MOV T[[RW_GPR]].Y, 0.0
; EG-CHECK: MOV * T[[RW_GPR]].Z, 0.0

; SI-CHECK-LABEL: @store_i8
; SI-CHECK: BUFFER_STORE_BYTE

define void @store_i8(i8 addrspace(1)* %out, i8 %in) {
entry:
  store i8 %in, i8 addrspace(1)* %out
  ret void
}

; i16 store
; EG-CHECK-LABEL: @store_i16
; EG-CHECK: MEM_RAT MSKOR T[[RW_GPR:[0-9]]].XW, T{{[0-9]}}.X
; EG-CHECK: VTX_READ_16 [[VAL:T[0-9]\.X]], [[VAL]]
; IG 0: Get the byte index and truncate the value
; EG-CHECK: AND_INT T{{[0-9]}}.[[BI_CHAN:[XYZW]]], KC0[2].Y, literal.x
; EG-CHECK: AND_INT * T{{[0-9]}}.[[TRUNC_CHAN:[XYZW]]], [[VAL]], literal.y
; EG-CHECK-NEXT: 3(4.203895e-45), 65535(9.183409e-41)
; IG 1: Truncate the calculated the shift amount for the mask
; EG-CHECK: LSHL * T{{[0-9]}}.[[SHIFT_CHAN:[XYZW]]], PV.[[BI_CHAN]], literal.x
; EG-CHECK: 3
; IG 2: Shift the value and the mask
; EG-CHECK: LSHL T[[RW_GPR]].X, T{{[0-9]}}.[[TRUNC_CHAN]], PV.[[SHIFT_CHAN]]
; EG-CHECK: LSHL * T[[RW_GPR]].W, literal.x, PV.[[SHIFT_CHAN]]
; EG-CHECK-NEXT: 65535
; IG 3: Initialize the Y and Z channels to zero
;       XXX: An optimal scheduler should merge this into one of the prevous IGs.
; EG-CHECK: MOV T[[RW_GPR]].Y, 0.0
; EG-CHECK: MOV * T[[RW_GPR]].Z, 0.0

; SI-CHECK-LABEL: @store_i16
; SI-CHECK: BUFFER_STORE_SHORT
define void @store_i16(i16 addrspace(1)* %out, i16 %in) {
entry:
  store i16 %in, i16 addrspace(1)* %out
  ret void
}

; EG-CHECK-LABEL: @store_v2i8
; EG-CHECK: MEM_RAT MSKOR
; EG-CHECK-NOT: MEM_RAT MSKOR
; SI-CHECK-LABEL: @store_v2i8
; SI-CHECK: BUFFER_STORE_BYTE
; SI-CHECK: BUFFER_STORE_BYTE
define void @store_v2i8(<2 x i8> addrspace(1)* %out, <2 x i32> %in) {
entry:
  %0 = trunc <2 x i32> %in to <2 x i8>
  store <2 x i8> %0, <2 x i8> addrspace(1)* %out
  ret void
}


; EG-CHECK-LABEL: @store_v2i16
; EG-CHECK: MEM_RAT_CACHELESS STORE_RAW
; CM-CHECK-LABEL: @store_v2i16
; CM-CHECK: MEM_RAT_CACHELESS STORE_DWORD
; SI-CHECK-LABEL: @store_v2i16
; SI-CHECK: BUFFER_STORE_SHORT
; SI-CHECK: BUFFER_STORE_SHORT
define void @store_v2i16(<2 x i16> addrspace(1)* %out, <2 x i32> %in) {
entry:
  %0 = trunc <2 x i32> %in to <2 x i16>
  store <2 x i16> %0, <2 x i16> addrspace(1)* %out
  ret void
}

; EG-CHECK-LABEL: @store_v4i8
; EG-CHECK: MEM_RAT_CACHELESS STORE_RAW
; CM-CHECK-LABEL: @store_v4i8
; CM-CHECK: MEM_RAT_CACHELESS STORE_DWORD
; SI-CHECK-LABEL: @store_v4i8
; SI-CHECK: BUFFER_STORE_BYTE
; SI-CHECK: BUFFER_STORE_BYTE
; SI-CHECK: BUFFER_STORE_BYTE
; SI-CHECK: BUFFER_STORE_BYTE
define void @store_v4i8(<4 x i8> addrspace(1)* %out, <4 x i32> %in) {
entry:
  %0 = trunc <4 x i32> %in to <4 x i8>
  store <4 x i8> %0, <4 x i8> addrspace(1)* %out
  ret void
}

; floating-point store
; EG-CHECK-LABEL: @store_f32
; EG-CHECK: MEM_RAT_CACHELESS STORE_RAW T{{[0-9]+\.X, T[0-9]+\.X}}, 1
; CM-CHECK-LABEL: @store_f32
; CM-CHECK: MEM_RAT_CACHELESS STORE_DWORD T{{[0-9]+\.X, T[0-9]+\.X}}
; SI-CHECK-LABEL: @store_f32
; SI-CHECK: BUFFER_STORE_DWORD

define void @store_f32(float addrspace(1)* %out, float %in) {
  store float %in, float addrspace(1)* %out
  ret void
}

; EG-CHECK-LABEL: @store_v4i16
; EG-CHECK: MEM_RAT MSKOR
; EG-CHECK: MEM_RAT MSKOR
; EG-CHECK: MEM_RAT MSKOR
; EG-CHECK: MEM_RAT MSKOR
; EG-CHECK-NOT: MEM_RAT MSKOR
; SI-CHECK-LABEL: @store_v4i16
; SI-CHECK: BUFFER_STORE_SHORT
; SI-CHECK: BUFFER_STORE_SHORT
; SI-CHECK: BUFFER_STORE_SHORT
; SI-CHECK: BUFFER_STORE_SHORT
; SI-CHECK-NOT: BUFFER_STORE_BYTE
define void @store_v4i16(<4 x i16> addrspace(1)* %out, <4 x i32> %in) {
entry:
  %0 = trunc <4 x i32> %in to <4 x i16>
  store <4 x i16> %0, <4 x i16> addrspace(1)* %out
  ret void
}

; vec2 floating-point stores
; EG-CHECK-LABEL: @store_v2f32
; EG-CHECK: MEM_RAT_CACHELESS STORE_RAW
; CM-CHECK-LABEL: @store_v2f32
; CM-CHECK: MEM_RAT_CACHELESS STORE_DWORD
; SI-CHECK-LABEL: @store_v2f32
; SI-CHECK: BUFFER_STORE_DWORDX2

define void @store_v2f32(<2 x float> addrspace(1)* %out, float %a, float %b) {
entry:
  %0 = insertelement <2 x float> <float 0.0, float 0.0>, float %a, i32 0
  %1 = insertelement <2 x float> %0, float %b, i32 1
  store <2 x float> %1, <2 x float> addrspace(1)* %out
  ret void
}

; EG-CHECK-LABEL: @store_v4i32
; EG-CHECK: MEM_RAT_CACHELESS STORE_RAW
; EG-CHECK-NOT: MEM_RAT_CACHELESS STORE_RAW
; CM-CHECK-LABEL: @store_v4i32
; CM-CHECK: MEM_RAT_CACHELESS STORE_DWORD
; CM-CHECK-NOT: MEM_RAT_CACHELESS STORE_DWORD
; SI-CHECK-LABEL: @store_v4i32
; SI-CHECK: BUFFER_STORE_DWORDX4
define void @store_v4i32(<4 x i32> addrspace(1)* %out, <4 x i32> %in) {
entry:
  store <4 x i32> %in, <4 x i32> addrspace(1)* %out
  ret void
}

;===------------------------------------------------------------------------===;
; Local Address Space
;===------------------------------------------------------------------------===;

; EG-CHECK-LABEL: @store_local_i8
; EG-CHECK: LDS_BYTE_WRITE
; SI-CHECK-LABEL: @store_local_i8
; SI-CHECK: DS_WRITE_B8
define void @store_local_i8(i8 addrspace(3)* %out, i8 %in) {
  store i8 %in, i8 addrspace(3)* %out
  ret void
}

; EG-CHECK-LABEL: @store_local_i16
; EG-CHECK: LDS_SHORT_WRITE
; SI-CHECK-LABEL: @store_local_i16
; SI-CHECK: DS_WRITE_B16
define void @store_local_i16(i16 addrspace(3)* %out, i16 %in) {
  store i16 %in, i16 addrspace(3)* %out
  ret void
}

; EG-CHECK-LABEL: @store_local_v2i16
; EG-CHECK: LDS_WRITE
; CM-CHECK-LABEL: @store_local_v2i16
; CM-CHECK: LDS_WRITE
; SI-CHECK-LABEL: @store_local_v2i16
; SI-CHECK: DS_WRITE_B16
; SI-CHECK: DS_WRITE_B16
define void @store_local_v2i16(<2 x i16> addrspace(3)* %out, <2 x i16> %in) {
entry:
  store <2 x i16> %in, <2 x i16> addrspace(3)* %out
  ret void
}

; EG-CHECK-LABEL: @store_local_v4i8
; EG-CHECK: LDS_WRITE
; CM-CHECK-LABEL: @store_local_v4i8
; CM-CHECK: LDS_WRITE
; SI-CHECK-LABEL: @store_local_v4i8
; SI-CHECK: DS_WRITE_B8
; SI-CHECK: DS_WRITE_B8
; SI-CHECK: DS_WRITE_B8
; SI-CHECK: DS_WRITE_B8
define void @store_local_v4i8(<4 x i8> addrspace(3)* %out, <4 x i8> %in) {
entry:
  store <4 x i8> %in, <4 x i8> addrspace(3)* %out
  ret void
}

; EG-CHECK-LABEL: @store_local_v2i32
; EG-CHECK: LDS_WRITE
; EG-CHECK: LDS_WRITE
; CM-CHECK-LABEL: @store_local_v2i32
; CM-CHECK: LDS_WRITE
; CM-CHECK: LDS_WRITE
; SI-CHECK-LABEL: @store_local_v2i32
; SI-CHECK: DS_WRITE_B32
; SI-CHECK: DS_WRITE_B32
define void @store_local_v2i32(<2 x i32> addrspace(3)* %out, <2 x i32> %in) {
entry:
  store <2 x i32> %in, <2 x i32> addrspace(3)* %out
  ret void
}

; EG-CHECK-LABEL: @store_local_v4i32
; EG-CHECK: LDS_WRITE
; EG-CHECK: LDS_WRITE
; EG-CHECK: LDS_WRITE
; EG-CHECK: LDS_WRITE
; CM-CHECK-LABEL: @store_local_v4i32
; CM-CHECK: LDS_WRITE
; CM-CHECK: LDS_WRITE
; CM-CHECK: LDS_WRITE
; CM-CHECK: LDS_WRITE
; SI-CHECK-LABEL: @store_local_v4i32
; SI-CHECK: DS_WRITE_B32
; SI-CHECK: DS_WRITE_B32
; SI-CHECK: DS_WRITE_B32
; SI-CHECK: DS_WRITE_B32
define void @store_local_v4i32(<4 x i32> addrspace(3)* %out, <4 x i32> %in) {
entry:
  store <4 x i32> %in, <4 x i32> addrspace(3)* %out
  ret void
}

; The stores in this function are combined by the optimizer to create a
; 64-bit store with 32-bit alignment.  This is legal for SI and the legalizer
; should not try to split the 64-bit store back into 2 32-bit stores.
;
; Evergreen / Northern Islands don't support 64-bit stores yet, so there should
; be two 32-bit stores.

; EG-CHECK-LABEL: @vecload2
; EG-CHECK: MEM_RAT_CACHELESS STORE_RAW
; CM-CHECK-LABEL: @vecload2
; CM-CHECK: MEM_RAT_CACHELESS STORE_DWORD
; SI-CHECK-LABEL: @vecload2
; SI-CHECK: BUFFER_STORE_DWORDX2
define void @vecload2(i32 addrspace(1)* nocapture %out, i32 addrspace(2)* nocapture %mem) #0 {
entry:
  %0 = load i32 addrspace(2)* %mem, align 4
  %arrayidx1.i = getelementptr inbounds i32 addrspace(2)* %mem, i64 1
  %1 = load i32 addrspace(2)* %arrayidx1.i, align 4
  store i32 %0, i32 addrspace(1)* %out, align 4
  %arrayidx1 = getelementptr inbounds i32 addrspace(1)* %out, i64 1
  store i32 %1, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }

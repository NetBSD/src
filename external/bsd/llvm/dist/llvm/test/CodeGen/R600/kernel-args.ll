; RUN: llc < %s -march=r600 -mcpu=redwood | FileCheck %s --check-prefix=EG-CHECK
; RUN: llc < %s -march=r600 -mcpu=cayman | FileCheck %s --check-prefix=EG-CHECK
; RUN: llc < %s -march=amdgcn -mcpu=SI -verify-machineinstrs | FileCheck %s --check-prefix=SI-CHECK

; EG-CHECK-LABEL: {{^}}i8_arg:
; EG-CHECK: MOV {{[ *]*}}T{{[0-9]+\.[XYZW]}}, KC0[2].Z
; SI-CHECK-LABEL: {{^}}i8_arg:
; SI-CHECK: buffer_load_ubyte

define void @i8_arg(i32 addrspace(1)* nocapture %out, i8 %in) nounwind {
entry:
  %0 = zext i8 %in to i32
  store i32 %0, i32 addrspace(1)* %out, align 4
  ret void
}

; EG-CHECK-LABEL: {{^}}i8_zext_arg:
; EG-CHECK: MOV {{[ *]*}}T{{[0-9]+\.[XYZW]}}, KC0[2].Z
; SI-CHECK-LABEL: {{^}}i8_zext_arg:
; SI-CHECK: s_load_dword s{{[0-9]}}, s[0:1], 0xb

define void @i8_zext_arg(i32 addrspace(1)* nocapture %out, i8 zeroext %in) nounwind {
entry:
  %0 = zext i8 %in to i32
  store i32 %0, i32 addrspace(1)* %out, align 4
  ret void
}

; EG-CHECK-LABEL: {{^}}i8_sext_arg:
; EG-CHECK: MOV {{[ *]*}}T{{[0-9]+\.[XYZW]}}, KC0[2].Z
; SI-CHECK-LABEL: {{^}}i8_sext_arg:
; SI-CHECK: s_load_dword s{{[0-9]}}, s[0:1], 0xb

define void @i8_sext_arg(i32 addrspace(1)* nocapture %out, i8 signext %in) nounwind {
entry:
  %0 = sext i8 %in to i32
  store i32 %0, i32 addrspace(1)* %out, align 4
  ret void
}

; EG-CHECK-LABEL: {{^}}i16_arg:
; EG-CHECK: MOV {{[ *]*}}T{{[0-9]+\.[XYZW]}}, KC0[2].Z
; SI-CHECK-LABEL: {{^}}i16_arg:
; SI-CHECK: buffer_load_ushort

define void @i16_arg(i32 addrspace(1)* nocapture %out, i16 %in) nounwind {
entry:
  %0 = zext i16 %in to i32
  store i32 %0, i32 addrspace(1)* %out, align 4
  ret void
}

; EG-CHECK-LABEL: {{^}}i16_zext_arg:
; EG-CHECK: MOV {{[ *]*}}T{{[0-9]+\.[XYZW]}}, KC0[2].Z
; SI-CHECK-LABEL: {{^}}i16_zext_arg:
; SI-CHECK: s_load_dword s{{[0-9]}}, s[0:1], 0xb

define void @i16_zext_arg(i32 addrspace(1)* nocapture %out, i16 zeroext %in) nounwind {
entry:
  %0 = zext i16 %in to i32
  store i32 %0, i32 addrspace(1)* %out, align 4
  ret void
}

; EG-CHECK-LABEL: {{^}}i16_sext_arg:
; EG-CHECK: MOV {{[ *]*}}T{{[0-9]+\.[XYZW]}}, KC0[2].Z
; SI-CHECK-LABEL: {{^}}i16_sext_arg:
; SI-CHECK: s_load_dword s{{[0-9]}}, s[0:1], 0xb

define void @i16_sext_arg(i32 addrspace(1)* nocapture %out, i16 signext %in) nounwind {
entry:
  %0 = sext i16 %in to i32
  store i32 %0, i32 addrspace(1)* %out, align 4
  ret void
}

; EG-CHECK-LABEL: {{^}}i32_arg:
; EG-CHECK: T{{[0-9]\.[XYZW]}}, KC0[2].Z
; SI-CHECK-LABEL: {{^}}i32_arg:
; s_load_dword s{{[0-9]}}, s[0:1], 0xb
define void @i32_arg(i32 addrspace(1)* nocapture %out, i32 %in) nounwind {
entry:
  store i32 %in, i32 addrspace(1)* %out, align 4
  ret void
}

; EG-CHECK-LABEL: {{^}}f32_arg:
; EG-CHECK: T{{[0-9]\.[XYZW]}}, KC0[2].Z
; SI-CHECK-LABEL: {{^}}f32_arg:
; s_load_dword s{{[0-9]}}, s[0:1], 0xb
define void @f32_arg(float addrspace(1)* nocapture %out, float %in) nounwind {
entry:
  store float %in, float addrspace(1)* %out, align 4
  ret void
}

; EG-CHECK-LABEL: {{^}}v2i8_arg:
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; SI-CHECK-LABEL: {{^}}v2i8_arg:
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
define void @v2i8_arg(<2 x i8> addrspace(1)* %out, <2 x i8> %in) {
entry:
  store <2 x i8> %in, <2 x i8> addrspace(1)* %out
  ret void
}

; EG-CHECK-LABEL: {{^}}v2i16_arg:
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; SI-CHECK-LABEL: {{^}}v2i16_arg:
; SI-CHECK-DAG: buffer_load_ushort
; SI-CHECK-DAG: buffer_load_ushort
define void @v2i16_arg(<2 x i16> addrspace(1)* %out, <2 x i16> %in) {
entry:
  store <2 x i16> %in, <2 x i16> addrspace(1)* %out
  ret void
}

; EG-CHECK-LABEL: {{^}}v2i32_arg:
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[3].X
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[2].W
; SI-CHECK-LABEL: {{^}}v2i32_arg:
; SI-CHECK: s_load_dwordx2 s{{\[[0-9]:[0-9]\]}}, s[0:1], 0xb
define void @v2i32_arg(<2 x i32> addrspace(1)* nocapture %out, <2 x i32> %in) nounwind {
entry:
  store <2 x i32> %in, <2 x i32> addrspace(1)* %out, align 4
  ret void
}

; EG-CHECK-LABEL: {{^}}v2f32_arg:
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[3].X
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[2].W
; SI-CHECK-LABEL: {{^}}v2f32_arg:
; SI-CHECK: s_load_dwordx2 s{{\[[0-9]:[0-9]\]}}, s[0:1], 0xb
define void @v2f32_arg(<2 x float> addrspace(1)* nocapture %out, <2 x float> %in) nounwind {
entry:
  store <2 x float> %in, <2 x float> addrspace(1)* %out, align 4
  ret void
}

; EG-CHECK-LABEL: {{^}}v3i8_arg:
; VTX_READ_8 T{{[0-9]}}.X, T{{[0-9]}}.X, 40
; VTX_READ_8 T{{[0-9]}}.X, T{{[0-9]}}.X, 41
; VTX_READ_8 T{{[0-9]}}.X, T{{[0-9]}}.X, 42
; SI-CHECK-LABEL: {{^}}v3i8_arg:
define void @v3i8_arg(<3 x i8> addrspace(1)* nocapture %out, <3 x i8> %in) nounwind {
entry:
  store <3 x i8> %in, <3 x i8> addrspace(1)* %out, align 4
  ret void
}

; EG-CHECK-LABEL: {{^}}v3i16_arg:
; VTX_READ_16 T{{[0-9]}}.X, T{{[0-9]}}.X, 44
; VTX_READ_16 T{{[0-9]}}.X, T{{[0-9]}}.X, 46
; VTX_READ_16 T{{[0-9]}}.X, T{{[0-9]}}.X, 48
; SI-CHECK-LABEL: {{^}}v3i16_arg:
define void @v3i16_arg(<3 x i16> addrspace(1)* nocapture %out, <3 x i16> %in) nounwind {
entry:
  store <3 x i16> %in, <3 x i16> addrspace(1)* %out, align 4
  ret void
}
; EG-CHECK-LABEL: {{^}}v3i32_arg:
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[3].Y
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[3].Z
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[3].W
; SI-CHECK-LABEL: {{^}}v3i32_arg:
; SI-CHECK: s_load_dwordx4 s{{\[[0-9]:[0-9]+\]}}, s[0:1], 0xd
define void @v3i32_arg(<3 x i32> addrspace(1)* nocapture %out, <3 x i32> %in) nounwind {
entry:
  store <3 x i32> %in, <3 x i32> addrspace(1)* %out, align 4
  ret void
}

; EG-CHECK-LABEL: {{^}}v3f32_arg:
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[3].Y
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[3].Z
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[3].W
; SI-CHECK-LABEL: {{^}}v3f32_arg:
; SI-CHECK: s_load_dwordx4 s{{\[[0-9]:[0-9]+\]}}, s[0:1], 0xd
define void @v3f32_arg(<3 x float> addrspace(1)* nocapture %out, <3 x float> %in) nounwind {
entry:
  store <3 x float> %in, <3 x float> addrspace(1)* %out, align 4
  ret void
}

; EG-CHECK-LABEL: {{^}}v4i8_arg:
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; SI-CHECK-LABEL: {{^}}v4i8_arg:
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
define void @v4i8_arg(<4 x i8> addrspace(1)* %out, <4 x i8> %in) {
entry:
  store <4 x i8> %in, <4 x i8> addrspace(1)* %out
  ret void
}

; EG-CHECK-LABEL: {{^}}v4i16_arg:
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; SI-CHECK-LABEL: {{^}}v4i16_arg:
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
define void @v4i16_arg(<4 x i16> addrspace(1)* %out, <4 x i16> %in) {
entry:
  store <4 x i16> %in, <4 x i16> addrspace(1)* %out
  ret void
}

; EG-CHECK-LABEL: {{^}}v4i32_arg:
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[3].Y
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[3].Z
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[3].W
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[4].X
; SI-CHECK-LABEL: {{^}}v4i32_arg:
; SI-CHECK: s_load_dwordx4 s{{\[[0-9]:[0-9]\]}}, s[0:1], 0xd
define void @v4i32_arg(<4 x i32> addrspace(1)* nocapture %out, <4 x i32> %in) nounwind {
entry:
  store <4 x i32> %in, <4 x i32> addrspace(1)* %out, align 4
  ret void
}

; EG-CHECK-LABEL: {{^}}v4f32_arg:
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[3].Y
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[3].Z
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[3].W
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[4].X
; SI-CHECK-LABEL: {{^}}v4f32_arg:
; SI-CHECK: s_load_dwordx4 s{{\[[0-9]:[0-9]\]}}, s[0:1], 0xd
define void @v4f32_arg(<4 x float> addrspace(1)* nocapture %out, <4 x float> %in) nounwind {
entry:
  store <4 x float> %in, <4 x float> addrspace(1)* %out, align 4
  ret void
}

; EG-CHECK-LABEL: {{^}}v8i8_arg:
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; SI-CHECK-LABEL: {{^}}v8i8_arg:
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
define void @v8i8_arg(<8 x i8> addrspace(1)* %out, <8 x i8> %in) {
entry:
  store <8 x i8> %in, <8 x i8> addrspace(1)* %out
  ret void
}

; EG-CHECK-LABEL: {{^}}v8i16_arg:
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; SI-CHECK-LABEL: {{^}}v8i16_arg:
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
define void @v8i16_arg(<8 x i16> addrspace(1)* %out, <8 x i16> %in) {
entry:
  store <8 x i16> %in, <8 x i16> addrspace(1)* %out
  ret void
}

; EG-CHECK-LABEL: {{^}}v8i32_arg:
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[4].Y
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[4].Z
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[4].W
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[5].X
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[5].Y
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[5].Z
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[5].W
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[6].X
; SI-CHECK-LABEL: {{^}}v8i32_arg:
; SI-CHECK: s_load_dwordx8 s{{\[[0-9]:[0-9]+\]}}, s[0:1], 0x11
define void @v8i32_arg(<8 x i32> addrspace(1)* nocapture %out, <8 x i32> %in) nounwind {
entry:
  store <8 x i32> %in, <8 x i32> addrspace(1)* %out, align 4
  ret void
}

; EG-CHECK-LABEL: {{^}}v8f32_arg:
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[4].Y
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[4].Z
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[4].W
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[5].X
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[5].Y
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[5].Z
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[5].W
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[6].X
; SI-CHECK-LABEL: {{^}}v8f32_arg:
; SI-CHECK: s_load_dwordx8 s{{\[[0-9]:[0-9]+\]}}, s[0:1], 0x11
define void @v8f32_arg(<8 x float> addrspace(1)* nocapture %out, <8 x float> %in) nounwind {
entry:
  store <8 x float> %in, <8 x float> addrspace(1)* %out, align 4
  ret void
}

; EG-CHECK-LABEL: {{^}}v16i8_arg:
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; EG-CHECK: VTX_READ_8
; SI-CHECK-LABEL: {{^}}v16i8_arg:
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
; SI-CHECK: buffer_load_ubyte
define void @v16i8_arg(<16 x i8> addrspace(1)* %out, <16 x i8> %in) {
entry:
  store <16 x i8> %in, <16 x i8> addrspace(1)* %out
  ret void
}

; EG-CHECK-LABEL: {{^}}v16i16_arg:
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; EG-CHECK: VTX_READ_16
; SI-CHECK-LABEL: {{^}}v16i16_arg:
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
; SI-CHECK: buffer_load_ushort
define void @v16i16_arg(<16 x i16> addrspace(1)* %out, <16 x i16> %in) {
entry:
  store <16 x i16> %in, <16 x i16> addrspace(1)* %out
  ret void
}

; EG-CHECK-LABEL: {{^}}v16i32_arg:
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[6].Y
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[6].Z
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[6].W
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[7].X
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[7].Y
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[7].Z
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[7].W
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[8].X
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[8].Y
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[8].Z
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[8].W
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[9].X
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[9].Y
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[9].Z
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[9].W
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[10].X
; SI-CHECK-LABEL: {{^}}v16i32_arg:
; SI-CHECK: s_load_dwordx16 s{{\[[0-9]:[0-9]+\]}}, s[0:1], 0x19
define void @v16i32_arg(<16 x i32> addrspace(1)* nocapture %out, <16 x i32> %in) nounwind {
entry:
  store <16 x i32> %in, <16 x i32> addrspace(1)* %out, align 4
  ret void
}

; EG-CHECK-LABEL: {{^}}v16f32_arg:
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[6].Y
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[6].Z
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[6].W
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[7].X
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[7].Y
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[7].Z
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[7].W
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[8].X
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[8].Y
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[8].Z
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[8].W
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[9].X
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[9].Y
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[9].Z
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[9].W
; EG-CHECK-DAG: T{{[0-9]\.[XYZW]}}, KC0[10].X
; SI-CHECK-LABEL: {{^}}v16f32_arg:
; SI-CHECK: s_load_dwordx16 s{{\[[0-9]:[0-9]+\]}}, s[0:1], 0x19
define void @v16f32_arg(<16 x float> addrspace(1)* nocapture %out, <16 x float> %in) nounwind {
entry:
  store <16 x float> %in, <16 x float> addrspace(1)* %out, align 4
  ret void
}

; FUNC-LABEL: {{^}}kernel_arg_i64:
; SI: s_load_dwordx2
; SI: s_load_dwordx2
; SI: buffer_store_dwordx2
define void @kernel_arg_i64(i64 addrspace(1)* %out, i64 %a) nounwind {
  store i64 %a, i64 addrspace(1)* %out, align 8
  ret void
}

; XFUNC-LABEL: {{^}}kernel_arg_v1i64:
; XSI: s_load_dwordx2
; XSI: s_load_dwordx2
; XSI: buffer_store_dwordx2
; define void @kernel_arg_v1i64(<1 x i64> addrspace(1)* %out, <1 x i64> %a) nounwind {
;   store <1 x i64> %a, <1 x i64> addrspace(1)* %out, align 8
;   ret void
; }

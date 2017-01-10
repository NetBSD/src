; RUN: llc -march=amdgcn -mcpu=verde -verify-machineinstrs < %s | FileCheck -check-prefix=SI -check-prefix=GCN %s
; RUN: llc -march=amdgcn -mcpu=tonga -verify-machineinstrs < %s | FileCheck -check-prefix=VI -check-prefix=GCN %s

; GCN-LABEL: {{^}}main:
; GCN: s_mov_b32 m0, s0
; VI-NEXT: s_nop 0
; GCN-NEXT: sendmsg(MSG_GS_DONE, GS_OP_NOP)
; GCN-NEXT: s_endpgm

define amdgpu_gs void @main(i32 inreg %a) #0 {
  call void @llvm.amdgcn.s.sendmsg(i32 3, i32 %a)
  ret void
}

; GCN-LABEL: {{^}}main_halt:
; GCN: s_mov_b32 m0, s0
; VI-NEXT: s_nop 0
; GCN-NEXT: s_sendmsghalt sendmsg(MSG_INTERRUPT)
; GCN-NEXT: s_endpgm

define  void @main_halt(i32 inreg %a) #0 {
  call void @llvm.amdgcn.s.sendmsghalt(i32 1, i32 %a)
  ret void
}

; GCN-LABEL: {{^}}legacy:
; GCN: s_mov_b32 m0, s0
; VI-NEXT: s_nop 0
; GCN-NEXT: sendmsg(MSG_GS_DONE, GS_OP_NOP)
; GCN-NEXT: s_endpgm

define amdgpu_gs void @legacy(i32 inreg %a) #0 {
  call void @llvm.SI.sendmsg(i32 3, i32 %a)
  ret void
}

declare void @llvm.amdgcn.s.sendmsg(i32, i32) #0
declare void @llvm.amdgcn.s.sendmsghalt(i32, i32) #0
declare void @llvm.SI.sendmsg(i32, i32) #0

attributes #0 = { nounwind }

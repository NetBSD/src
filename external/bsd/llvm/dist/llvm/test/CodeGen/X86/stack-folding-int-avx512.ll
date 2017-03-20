; RUN: llc -O3 -disable-peephole -mtriple=x86_64-unknown-unknown -mattr=+avx512f,+avx512bw,+avx512dq,+avx512vbmi < %s | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-unknown"

; Stack reload folding tests.
;
; By including a nop call with sideeffects we can force a partial register spill of the
; relevant registers and check that the reload is correctly folded into the instruction.

define <64 x i8> @stack_fold_paddb(<64 x i8> %a0, <64 x i8> %a1) {
  ;CHECK-LABEL: stack_fold_paddb
  ;CHECK:       vpaddb {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = add <64 x i8> %a0, %a1
  ret <64 x i8> %2
}

define <64 x i8> @stack_fold_paddb_mask(<64 x i8> %a0, <64 x i8> %a1, <64 x i8>* %a2, i64 %mask) {
  ;CHECK-LABEL: stack_fold_paddb_mask
  ;CHECK:       vpaddb {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm0},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = add <64 x i8> %a0, %a1
  %3 = bitcast i64 %mask to <64 x i1>
  ; load needed to keep the operation from being scheduled about the asm block
  %4 = load <64 x i8>, <64 x i8>* %a2
  %5 = select <64 x i1> %3, <64 x i8> %2, <64 x i8> %4
  ret <64 x i8> %5
}

define <64 x i8> @stack_fold_paddb_maskz(<64 x i8> %a0, <64 x i8> %a1, i64 %mask) {
  ;CHECK-LABEL: stack_fold_paddb_maskz
  ;CHECK:       vpaddb {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {z} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = add <64 x i8> %a0, %a1
  %3 = bitcast i64 %mask to <64 x i1>
  %4 = select <64 x i1> %3, <64 x i8> %2, <64 x i8> zeroinitializer
  ret <64 x i8> %4
}

define <16 x i32> @stack_fold_paddd(<16 x i32> %a0, <16 x i32> %a1) {
  ;CHECK-LABEL: stack_fold_paddd
  ;CHECK:       vpaddd {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = add <16 x i32> %a0, %a1
  ret <16 x i32> %2
}

define <8 x i64> @stack_fold_paddq(<8 x i64> %a0, <8 x i64> %a1) {
  ;CHECK-LABEL: stack_fold_paddq
  ;CHECK:       vpaddq {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = add <8 x i64> %a0, %a1
  ret <8 x i64> %2
}

define <64 x i8> @stack_fold_paddsb(<64 x i8> %a0, <64 x i8> %a1) {
  ;CHECK-LABEL: stack_fold_paddsb
  ;CHECK:       vpaddsb {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <64 x i8> @llvm.x86.avx512.mask.padds.b.512(<64 x i8> %a0, <64 x i8> %a1, <64 x i8> undef, i64 -1)
  ret <64 x i8> %2
}
declare <64 x i8> @llvm.x86.avx512.mask.padds.b.512(<64 x i8>, <64 x i8>, <64 x i8>, i64) nounwind readnone

define <32 x i16> @stack_fold_paddsw(<32 x i16> %a0, <32 x i16> %a1) {
  ;CHECK-LABEL: stack_fold_paddsw
  ;CHECK:       vpaddsw {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <32 x i16> @llvm.x86.avx512.mask.padds.w.512(<32 x i16> %a0, <32 x i16> %a1, <32 x i16> undef, i32 -1)
  ret <32 x i16> %2
}
declare <32 x i16> @llvm.x86.avx512.mask.padds.w.512(<32 x i16>, <32 x i16>, <32 x i16>, i32)

define <64 x i8> @stack_fold_paddusb(<64 x i8> %a0, <64 x i8> %a1) {
  ;CHECK-LABEL: stack_fold_paddusb
  ;CHECK:       vpaddusb {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <64 x i8> @llvm.x86.avx512.mask.paddus.b.512(<64 x i8> %a0, <64 x i8> %a1, <64 x i8> undef, i64 -1)
  ret <64 x i8> %2
}
declare <64 x i8> @llvm.x86.avx512.mask.paddus.b.512(<64 x i8>, <64 x i8>, <64 x i8>, i64) nounwind readnone

define <32 x i16> @stack_fold_paddusw(<32 x i16> %a0, <32 x i16> %a1) {
  ;CHECK-LABEL: stack_fold_paddusw
  ;CHECK:       vpaddusw {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <32 x i16> @llvm.x86.avx512.mask.paddus.w.512(<32 x i16> %a0, <32 x i16> %a1, <32 x i16> undef, i32 -1)
  ret <32 x i16> %2
}
declare <32 x i16> @llvm.x86.avx512.mask.paddus.w.512(<32 x i16>, <32 x i16>, <32 x i16>, i32)

define <32 x i16> @stack_fold_paddw(<32 x i16> %a0, <32 x i16> %a1) {
  ;CHECK-LABEL: stack_fold_paddw
  ;CHECK:       vpaddw {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = add <32 x i16> %a0, %a1
  ret <32 x i16> %2
}

define i64 @stack_fold_pcmpeqb(<64 x i8> %a0, <64 x i8> %a1) {
  ;CHECK-LABEL: stack_fold_pcmpeqb
  ;CHECK:       vpcmpeqb {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%k[0-7]}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = icmp eq <64 x i8> %a0, %a1
  %3 = bitcast <64 x i1> %2 to i64
  ret i64 %3
}

define i16 @stack_fold_pcmpeqd(<16 x i32> %a0, <16 x i32> %a1) {
  ;CHECK-LABEL: stack_fold_pcmpeqd
  ;CHECK:       vpcmpeqd {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%k[0-7]}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = icmp eq <16 x i32> %a0, %a1
  %3 = bitcast <16 x i1> %2 to i16
  ret i16 %3
}

define i8 @stack_fold_pcmpeqq(<8 x i64> %a0, <8 x i64> %a1) {
  ;CHECK-LABEL: stack_fold_pcmpeqq
  ;CHECK:       vpcmpeqq {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%k[0-7]}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = icmp eq <8 x i64> %a0, %a1
  %3 = bitcast <8 x i1> %2 to i8
  ret i8 %3
}

define i32 @stack_fold_pcmpeqw(<32 x i16> %a0, <32 x i16> %a1) {
  ;CHECK-LABEL: stack_fold_pcmpeqw
  ;CHECK:       vpcmpeqw {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%k[0-7]}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = icmp eq <32 x i16> %a0, %a1
  %3 = bitcast <32 x i1> %2 to i32
  ret i32 %3
}

define <64 x i8> @stack_fold_psubb(<64 x i8> %a0, <64 x i8> %a1) {
  ;CHECK-LABEL: stack_fold_psubb
  ;CHECK:       vpsubb {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = sub <64 x i8> %a0, %a1
  ret <64 x i8> %2
}

define <16 x i32> @stack_fold_psubd(<16 x i32> %a0, <16 x i32> %a1) {
  ;CHECK-LABEL: stack_fold_psubd
  ;CHECK:       vpsubd {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = sub <16 x i32> %a0, %a1
  ret <16 x i32> %2
}

define <8 x i64> @stack_fold_psubq(<8 x i64> %a0, <8 x i64> %a1) {
  ;CHECK-LABEL: stack_fold_psubq
  ;CHECK:       vpsubq {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = sub <8 x i64> %a0, %a1
  ret <8 x i64> %2
}

define <64 x i8> @stack_fold_psubsb(<64 x i8> %a0, <64 x i8> %a1) {
  ;CHECK-LABEL: stack_fold_psubsb
  ;CHECK:       vpsubsb {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <64 x i8> @llvm.x86.avx512.mask.psubs.b.512(<64 x i8> %a0, <64 x i8> %a1, <64 x i8> undef, i64 -1)
  ret <64 x i8> %2
}
declare <64 x i8> @llvm.x86.avx512.mask.psubs.b.512(<64 x i8>, <64 x i8>, <64 x i8>, i64) nounwind readnone

define <32 x i16> @stack_fold_psubsw(<32 x i16> %a0, <32 x i16> %a1) {
  ;CHECK-LABEL: stack_fold_psubsw
  ;CHECK:       vpsubsw {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <32 x i16> @llvm.x86.avx512.mask.psubs.w.512(<32 x i16> %a0, <32 x i16> %a1, <32 x i16> undef, i32 -1)
  ret <32 x i16> %2
}
declare <32 x i16> @llvm.x86.avx512.mask.psubs.w.512(<32 x i16>, <32 x i16>, <32 x i16>, i32)

define <64 x i8> @stack_fold_psubusb(<64 x i8> %a0, <64 x i8> %a1) {
  ;CHECK-LABEL: stack_fold_psubusb
  ;CHECK:       vpsubusb {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <64 x i8> @llvm.x86.avx512.mask.psubus.b.512(<64 x i8> %a0, <64 x i8> %a1, <64 x i8> undef, i64 -1)
  ret <64 x i8> %2
}
declare <64 x i8> @llvm.x86.avx512.mask.psubus.b.512(<64 x i8>, <64 x i8>, <64 x i8>, i64) nounwind readnone

define <32 x i16> @stack_fold_psubusw(<32 x i16> %a0, <32 x i16> %a1) {
  ;CHECK-LABEL: stack_fold_psubusw
  ;CHECK:       vpsubusw {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <32 x i16> @llvm.x86.avx512.mask.psubus.w.512(<32 x i16> %a0, <32 x i16> %a1, <32 x i16> undef, i32 -1)
  ret <32 x i16> %2
}
declare <32 x i16> @llvm.x86.avx512.mask.psubus.w.512(<32 x i16>, <32 x i16>, <32 x i16>, i32)

define <32 x i16> @stack_fold_psubw(<32 x i16> %a0, <32 x i16> %a1) {
  ;CHECK-LABEL: stack_fold_psubw
  ;CHECK:       vpsubw {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = sub <32 x i16> %a0, %a1
  ret <32 x i16> %2
}

define <16 x i32> @stack_fold_ternlogd(<16 x i32> %x0, <16 x i32> %x1, <16 x i32> %x2) {
  ;CHECK-LABEL: stack_fold_ternlogd
  ;CHECK:       vpternlogd $33, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %res = call <16 x i32> @llvm.x86.avx512.mask.pternlog.d.512(<16 x i32> %x0, <16 x i32> %x1, <16 x i32> %x2, i32 33, i16 -1)
  ret <16 x i32> %res
}
declare <16 x i32> @llvm.x86.avx512.mask.pternlog.d.512(<16 x i32>, <16 x i32>, <16 x i32>, i32, i16)

define <8 x i64> @stack_fold_ternlogq(<8 x i64> %x0, <8 x i64> %x1, <8 x i64> %x2) {
  ;CHECK-LABEL: stack_fold_ternlogq
  ;CHECK:       vpternlogq $33, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %res = call <8 x i64> @llvm.x86.avx512.mask.pternlog.q.512(<8 x i64> %x0, <8 x i64> %x1, <8 x i64> %x2, i32 33, i8 -1)
  ret <8 x i64> %res
}

declare <8 x i64> @llvm.x86.avx512.mask.pternlog.q.512(<8 x i64>, <8 x i64>, <8 x i64>, i32, i8)

define <16 x i8> @stack_fold_vpmovdb(<16 x i32> %a0) {
  ;CHECK-LABEL: stack_fold_vpmovdb
  ;CHECK:       vpmovdb %zmm0, {{-?[0-9]*}}(%rsp) # 16-byte Folded Spill
  %1 = call <16 x i8> @llvm.x86.avx512.mask.pmov.db.512(<16 x i32> %a0, <16 x i8> undef, i16 -1)
  %2 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  ret <16 x i8> %1
}
declare <16 x i8> @llvm.x86.avx512.mask.pmov.db.512(<16 x i32>, <16 x i8>, i16)

define <16 x i16> @stack_fold_vpmovdw(<16 x i32> %a0) {
  ;CHECK-LABEL: stack_fold_vpmovdw
  ;CHECK:       vpmovdw %zmm0, {{-?[0-9]*}}(%rsp) # 32-byte Folded Spill
  %1 = call <16 x i16> @llvm.x86.avx512.mask.pmov.dw.512(<16 x i32> %a0, <16 x i16> undef, i16 -1)
  %2 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  ret <16 x i16> %1
}
declare <16 x i16> @llvm.x86.avx512.mask.pmov.dw.512(<16 x i32>, <16 x i16>, i16)

define <8 x i32> @stack_fold_vpmovqd(<8 x i64> %a0) {
  ;CHECK-LABEL: stack_fold_vpmovqd
  ;CHECK:       vpmovqd %zmm0, {{-?[0-9]*}}(%rsp) # 32-byte Folded Spill
  %1 = call <8 x i32> @llvm.x86.avx512.mask.pmov.qd.512(<8 x i64> %a0, <8 x i32> undef, i8 -1)
  %2 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  ret <8 x i32> %1
}
declare <8 x i32> @llvm.x86.avx512.mask.pmov.qd.512(<8 x i64>, <8 x i32>, i8)

define <8 x i16> @stack_fold_vpmovqw(<8 x i64> %a0) {
  ;CHECK-LABEL: stack_fold_vpmovqw
  ;CHECK:       vpmovqw %zmm0, {{-?[0-9]*}}(%rsp) # 16-byte Folded Spill
  %1 = call <8 x i16> @llvm.x86.avx512.mask.pmov.qw.512(<8 x i64> %a0, <8 x i16> undef, i8 -1)
  %2 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  ret <8 x i16> %1
}
declare <8 x i16> @llvm.x86.avx512.mask.pmov.qw.512(<8 x i64>, <8 x i16>, i8)

define <32 x i8> @stack_fold_vpmovwb(<32 x i16> %a0) {
  ;CHECK-LABEL: stack_fold_vpmovwb
  ;CHECK:       vpmovwb %zmm0, {{-?[0-9]*}}(%rsp) # 32-byte Folded Spill
  %1 = call <32 x i8> @llvm.x86.avx512.mask.pmov.wb.512(<32 x i16> %a0, <32 x i8> undef, i32 -1)
  %2 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  ret <32 x i8> %1
}
declare <32 x i8> @llvm.x86.avx512.mask.pmov.wb.512(<32 x i16>, <32 x i8>, i32)

define <16 x i8> @stack_fold_vpmovsdb(<16 x i32> %a0) {
  ;CHECK-LABEL: stack_fold_vpmovsdb
  ;CHECK:       vpmovsdb %zmm0, {{-?[0-9]*}}(%rsp) # 16-byte Folded Spill
  %1 = call <16 x i8> @llvm.x86.avx512.mask.pmovs.db.512(<16 x i32> %a0, <16 x i8> undef, i16 -1)
  %2 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  ret <16 x i8> %1
}
declare <16 x i8> @llvm.x86.avx512.mask.pmovs.db.512(<16 x i32>, <16 x i8>, i16)

define <16 x i16> @stack_fold_vpmovsdw(<16 x i32> %a0) {
  ;CHECK-LABEL: stack_fold_vpmovsdw
  ;CHECK:       vpmovsdw %zmm0, {{-?[0-9]*}}(%rsp) # 32-byte Folded Spill
  %1 = call <16 x i16> @llvm.x86.avx512.mask.pmovs.dw.512(<16 x i32> %a0, <16 x i16> undef, i16 -1)
  %2 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  ret <16 x i16> %1
}
declare <16 x i16> @llvm.x86.avx512.mask.pmovs.dw.512(<16 x i32>, <16 x i16>, i16)

define <8 x i32> @stack_fold_vpmovsqd(<8 x i64> %a0) {
  ;CHECK-LABEL: stack_fold_vpmovsqd
  ;CHECK:       vpmovsqd %zmm0, {{-?[0-9]*}}(%rsp) # 32-byte Folded Spill
  %1 = call <8 x i32> @llvm.x86.avx512.mask.pmovs.qd.512(<8 x i64> %a0, <8 x i32> undef, i8 -1)
  %2 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  ret <8 x i32> %1
}
declare <8 x i32> @llvm.x86.avx512.mask.pmovs.qd.512(<8 x i64>, <8 x i32>, i8)

define <8 x i16> @stack_fold_vpmovsqw(<8 x i64> %a0) {
  ;CHECK-LABEL: stack_fold_vpmovsqw
  ;CHECK:       vpmovsqw %zmm0, {{-?[0-9]*}}(%rsp) # 16-byte Folded Spill
  %1 = call <8 x i16> @llvm.x86.avx512.mask.pmovs.qw.512(<8 x i64> %a0, <8 x i16> undef, i8 -1)
  %2 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  ret <8 x i16> %1
}
declare <8 x i16> @llvm.x86.avx512.mask.pmovs.qw.512(<8 x i64>, <8 x i16>, i8)

define <32 x i8> @stack_fold_vpmovswb(<32 x i16> %a0) {
  ;CHECK-LABEL: stack_fold_vpmovswb
  ;CHECK:       vpmovswb %zmm0, {{-?[0-9]*}}(%rsp) # 32-byte Folded Spill
  %1 = call <32 x i8> @llvm.x86.avx512.mask.pmovs.wb.512(<32 x i16> %a0, <32 x i8> undef, i32 -1)
  %2 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  ret <32 x i8> %1
}
declare <32 x i8> @llvm.x86.avx512.mask.pmovs.wb.512(<32 x i16>, <32 x i8>, i32)

define <16 x i8> @stack_fold_vpmovusdb(<16 x i32> %a0) {
  ;CHECK-LABEL: stack_fold_vpmovusdb
  ;CHECK:       vpmovusdb %zmm0, {{-?[0-9]*}}(%rsp) # 16-byte Folded Spill
  %1 = call <16 x i8> @llvm.x86.avx512.mask.pmovus.db.512(<16 x i32> %a0, <16 x i8> undef, i16 -1)
  %2 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  ret <16 x i8> %1
}
declare <16 x i8> @llvm.x86.avx512.mask.pmovus.db.512(<16 x i32>, <16 x i8>, i16)

define <16 x i16> @stack_fold_vpmovusdw(<16 x i32> %a0) {
  ;CHECK-LABEL: stack_fold_vpmovusdw
  ;CHECK:       vpmovusdw %zmm0, {{-?[0-9]*}}(%rsp) # 32-byte Folded Spill
  %1 = call <16 x i16> @llvm.x86.avx512.mask.pmovus.dw.512(<16 x i32> %a0, <16 x i16> undef, i16 -1)
  %2 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  ret <16 x i16> %1
}
declare <16 x i16> @llvm.x86.avx512.mask.pmovus.dw.512(<16 x i32>, <16 x i16>, i16)

define <8 x i32> @stack_fold_vpmovusqd(<8 x i64> %a0) {
  ;CHECK-LABEL: stack_fold_vpmovusqd
  ;CHECK:       vpmovusqd %zmm0, {{-?[0-9]*}}(%rsp) # 32-byte Folded Spill
  %1 = call <8 x i32> @llvm.x86.avx512.mask.pmovus.qd.512(<8 x i64> %a0, <8 x i32> undef, i8 -1)
  %2 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  ret <8 x i32> %1
}
declare <8 x i32> @llvm.x86.avx512.mask.pmovus.qd.512(<8 x i64>, <8 x i32>, i8)

define <8 x i16> @stack_fold_vpmovusqw(<8 x i64> %a0) {
  ;CHECK-LABEL: stack_fold_vpmovusqw
  ;CHECK:       vpmovusqw %zmm0, {{-?[0-9]*}}(%rsp) # 16-byte Folded Spill
  %1 = call <8 x i16> @llvm.x86.avx512.mask.pmovus.qw.512(<8 x i64> %a0, <8 x i16> undef, i8 -1)
  %2 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  ret <8 x i16> %1
}
declare <8 x i16> @llvm.x86.avx512.mask.pmovus.qw.512(<8 x i64>, <8 x i16>, i8)

define <32 x i8> @stack_fold_vpmovuswb(<32 x i16> %a0) {
  ;CHECK-LABEL: stack_fold_vpmovuswb
  ;CHECK:       vpmovuswb %zmm0, {{-?[0-9]*}}(%rsp) # 32-byte Folded Spill
  %1 = call <32 x i8> @llvm.x86.avx512.mask.pmovus.wb.512(<32 x i16> %a0, <32 x i8> undef, i32 -1)
  %2 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  ret <32 x i8> %1
}
declare <32 x i8> @llvm.x86.avx512.mask.pmovus.wb.512(<32 x i16>, <32 x i8>, i32)

define <4 x i32> @stack_fold_extracti32x4(<16 x i32> %a0, <16 x i32> %a1) {
  ;CHECK-LABEL: stack_fold_extracti32x4
  ;CHECK:       vextracti32x4 $3, {{%zmm[0-9][0-9]*}}, {{-?[0-9]*}}(%rsp) {{.*#+}} 16-byte Folded Spill
  ; add forces execution domain
  %1 = add <16 x i32> %a0, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %2 = shufflevector <16 x i32> %1, <16 x i32> %a1, <4 x i32> <i32 12, i32 13, i32 14, i32 15>
  %3 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  ret <4 x i32> %2
}

define <2 x i64> @stack_fold_extracti64x2(<8 x i64> %a0, <8 x i64> %a1) {
  ;CHECK-LABEL: stack_fold_extracti64x2
  ;CHECK:       vextracti64x2 $3, {{%zmm[0-9][0-9]*}}, {{-?[0-9]*}}(%rsp) {{.*#+}} 16-byte Folded Spill
  ; add forces execution domain
  %1 = add <8 x i64> %a0, <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  %2 = shufflevector <8 x i64> %1, <8 x i64> %a1, <2 x i32> <i32 6, i32 7>
  %3 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  ret <2 x i64> %2
}

define <8 x i32> @stack_fold_extracti32x8(<16 x i32> %a0, <16 x i32> %a1) {
  ;CHECK-LABEL: stack_fold_extracti32x8
  ;CHECK:       vextracti32x8 $1, {{%zmm[0-9][0-9]*}}, {{-?[0-9]*}}(%rsp) {{.*#+}} 32-byte Folded Spill
  ; add forces execution domain
  %1 = add <16 x i32> %a0, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %2 = shufflevector <16 x i32> %1, <16 x i32> %a1, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %3 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  ret <8 x i32> %2
}

define <4 x i64> @stack_fold_extracti64x4(<8 x i64> %a0, <8 x i64> %a1) {
  ;CHECK-LABEL: stack_fold_extracti64x4
  ;CHECK:       vextracti64x4 $1, {{%zmm[0-9][0-9]*}}, {{-?[0-9]*}}(%rsp) {{.*#+}} 32-byte Folded Spill
  ; add forces execution domain
  %1 = add <8 x i64> %a0, <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  %2 = shufflevector <8 x i64> %1, <8 x i64> %a1, <4 x i32> <i32 4, i32 5, i32 6, i32 7>
  %3 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  ret <4 x i64> %2
}

define <16 x i32> @stack_fold_inserti32x8(<8 x i32> %a0, <8 x i32> %a1) {
  ;CHECK-LABEL: stack_fold_inserti32x8
  ;CHECK:       vinserti32x8 $1, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 32-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <8 x i32> %a0, <8 x i32> %a1, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  ; add forces execution domain
  %3 = add <16 x i32> %2, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  ret <16 x i32> %3
}

define <8 x i64> @stack_fold_inserti64x4(<4 x i64> %a0, <4 x i64> %a1) {
  ;CHECK-LABEL: stack_fold_inserti64x4
  ;CHECK:       vinserti64x4 $1, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 32-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <4 x i64> %a0, <4 x i64> %a1, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  ; add forces execution domain
  %3 = add <8 x i64> %2, <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  ret <8 x i64> %3
}

define <2 x i64> @stack_fold_movq_load(<2 x i64> %a0) {
  ;CHECK-LABEL: stack_fold_movq_load
  ;CHECK:       movq {{-?[0-9]*}}(%rsp), {{%xmm[0-9][0-9]*}} {{.*#+}} 16-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <2 x i64> %a0, <2 x i64> zeroinitializer, <2 x i32> <i32 0, i32 2>
  ; add forces execution domain
  %3 = add <2 x i64> %2, <i64 1, i64 1>
  ret <2 x i64> %3
}

define <16 x i32> @stack_fold_vpermt2d(<16 x i32> %x0, <16 x i32> %x1, <16 x i32> %x2) {
  ;CHECK-LABEL: stack_fold_vpermt2d
  ;CHECK:       vpermt2d {{-?[0-9]*}}(%rsp), %zmm1, %zmm0 # 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %res = call <16 x i32> @llvm.x86.avx512.mask.vpermi2var.d.512(<16 x i32> %x0, <16 x i32> %x1, <16 x i32> %x2, i16 -1)
  ret <16 x i32> %res
}
declare <16 x i32> @llvm.x86.avx512.mask.vpermi2var.d.512(<16 x i32>, <16 x i32>, <16 x i32>, i16)

define <16 x i32> @stack_fold_vpermi2d(<16 x i32> %x0, <16 x i32> %x1, <16 x i32> %x2) {
  ;CHECK-LABEL: stack_fold_vpermi2d
  ;CHECK:       vpermi2d {{-?[0-9]*}}(%rsp), %zmm1, %zmm0 # 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %res = call <16 x i32> @llvm.x86.avx512.mask.vpermt2var.d.512(<16 x i32> %x0, <16 x i32> %x1, <16 x i32> %x2, i16 -1)
  ret <16 x i32> %res
}
declare <16 x i32> @llvm.x86.avx512.mask.vpermt2var.d.512(<16 x i32>, <16 x i32>, <16 x i32>, i16)

define <8 x i64> @stack_fold_vpermt2q(<8 x i64> %x0, <8 x i64> %x1, <8 x i64> %x2) {
  ;CHECK-LABEL: stack_fold_vpermt2q
  ;CHECK:       vpermt2q {{-?[0-9]*}}(%rsp), %zmm1, %zmm0 # 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %res = call <8 x i64> @llvm.x86.avx512.mask.vpermi2var.q.512(<8 x i64> %x0, <8 x i64> %x1, <8 x i64> %x2, i8 -1)
  ret <8 x i64> %res
}
declare <8 x i64> @llvm.x86.avx512.mask.vpermi2var.q.512(<8 x i64>, <8 x i64>, <8 x i64>, i8)

define <8 x i64> @stack_fold_vpermi2q(<8 x i64> %x0, <8 x i64> %x1, <8 x i64> %x2) {
  ;CHECK-LABEL: stack_fold_vpermi2q
  ;CHECK:       vpermi2q {{-?[0-9]*}}(%rsp), %zmm1, %zmm0 # 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %res = call <8 x i64> @llvm.x86.avx512.mask.vpermt2var.q.512(<8 x i64> %x0, <8 x i64> %x1, <8 x i64> %x2, i8 -1)
  ret <8 x i64> %res
}
declare <8 x i64> @llvm.x86.avx512.mask.vpermt2var.q.512(<8 x i64>, <8 x i64>, <8 x i64>, i8)

define <32 x i16> @stack_fold_vpermt2w(<32 x i16> %x0, <32 x i16> %x1, <32 x i16> %x2) {
  ;CHECK-LABEL: stack_fold_vpermt2w
  ;CHECK:       vpermt2w {{-?[0-9]*}}(%rsp), %zmm1, %zmm0 # 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %res = call <32 x i16> @llvm.x86.avx512.mask.vpermi2var.hi.512(<32 x i16> %x0, <32 x i16> %x1, <32 x i16> %x2, i32 -1)
  ret <32 x i16> %res
}
declare <32 x i16> @llvm.x86.avx512.mask.vpermi2var.hi.512(<32 x i16>, <32 x i16>, <32 x i16>, i32)

define <32 x i16> @stack_fold_vpermi2w(<32 x i16> %x0, <32 x i16> %x1, <32 x i16> %x2) {
  ;CHECK-LABEL: stack_fold_vpermi2w
  ;CHECK:       vpermi2w {{-?[0-9]*}}(%rsp), %zmm1, %zmm0 # 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %res = call <32 x i16> @llvm.x86.avx512.mask.vpermt2var.hi.512(<32 x i16> %x0, <32 x i16> %x1, <32 x i16> %x2, i32 -1)
  ret <32 x i16> %res
}
declare <32 x i16> @llvm.x86.avx512.mask.vpermt2var.hi.512(<32 x i16>, <32 x i16>, <32 x i16>, i32)

define <64 x i8> @stack_fold_vpermt2b(<64 x i8> %x0, <64 x i8> %x1, <64 x i8> %x2) {
  ;CHECK-LABEL: stack_fold_vpermt2b
  ;CHECK:       vpermt2b {{-?[0-9]*}}(%rsp), %zmm1, %zmm0 # 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %res = call <64 x i8> @llvm.x86.avx512.mask.vpermi2var.qi.512(<64 x i8> %x0, <64 x i8> %x1, <64 x i8> %x2, i64 -1)
  ret <64 x i8> %res
}
declare <64 x i8> @llvm.x86.avx512.mask.vpermi2var.qi.512(<64 x i8>, <64 x i8>, <64 x i8>, i64)

define <64 x i8> @stack_fold_vpermi2b(<64 x i8> %x0, <64 x i8> %x1, <64 x i8> %x2) {
  ;CHECK-LABEL: stack_fold_vpermi2b
  ;CHECK:       vpermi2b {{-?[0-9]*}}(%rsp), %zmm1, %zmm0 # 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %res = call <64 x i8> @llvm.x86.avx512.mask.vpermt2var.qi.512(<64 x i8> %x0, <64 x i8> %x1, <64 x i8> %x2, i64 -1)
  ret <64 x i8> %res
}
declare <64 x i8> @llvm.x86.avx512.mask.vpermt2var.qi.512(<64 x i8>, <64 x i8>, <64 x i8>, i64)

define <16 x i32> @stack_fold_pmovsxbd_zmm(<16 x i8> %a0) {
  ;CHECK-LABEL: stack_fold_pmovsxbd_zmm
  ;CHECK:       vpmovsxbd {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{.*#+}} 16-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = sext <16 x i8> %a0 to <16 x i32>
  ret <16 x i32> %2
}

define <8 x i64> @stack_fold_pmovsxbq_zmm(<16 x i8> %a0) {
  ;CHECK-LABEL: stack_fold_pmovsxbq_zmm
  ;CHECK:       pmovsxbq {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{.*#+}} 16-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <16 x i8> %a0, <16 x i8> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %3 = sext <8 x i8> %2 to <8 x i64>
  ret <8 x i64> %3
}

define <32 x i16> @stack_fold_pmovsxbw_zmm(<32 x i8> %a0) {
  ;CHECK-LABEL: stack_fold_pmovsxbw_zmm
  ;CHECK:       vpmovsxbw {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{.*#+}} 32-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = sext <32 x i8> %a0 to <32 x i16>
  ret <32 x i16> %2
}

define <8 x i64> @stack_fold_pmovsxdq_zmm(<8 x i32> %a0) {
  ;CHECK-LABEL: stack_fold_pmovsxdq_zmm
  ;CHECK:       vpmovsxdq {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{.*#+}} 32-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = sext <8 x i32> %a0 to <8 x i64>
  ret <8 x i64> %2
}

define <16 x i32> @stack_fold_pmovsxwd_zmm(<16 x i16> %a0) {
  ;CHECK-LABEL: stack_fold_pmovsxwd_zmm
  ;CHECK:       vpmovsxwd {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{.*#+}} 32-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = sext <16 x i16> %a0 to <16 x i32>
  ret <16 x i32> %2
}

define <8 x i64> @stack_fold_pmovsxwq_zmm(<8 x i16> %a0) {
  ;CHECK-LABEL: stack_fold_pmovsxwq_zmm
  ;CHECK:       vpmovsxwq {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{.*#+}} 16-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = sext <8 x i16> %a0 to <8 x i64>
  ret <8 x i64> %2
}

define <8 x i64> @stack_fold_pmovsxwq_mask_zmm(<8 x i64> %passthru, <8 x i16> %a0, i8 %mask) {
  ;CHECK-LABEL: stack_fold_pmovsxwq_mask_zmm
  ;CHECK:       vpmovsxwq {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {{.*#+}} 16-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = sext <8 x i16> %a0 to <8 x i64>
  %3 = bitcast i8 %mask to <8 x i1>
  %4 = select <8 x i1> %3, <8 x i64> %2, <8 x i64> %passthru
  ret <8 x i64> %4
}

define <8 x i64> @stack_fold_pmovsxwq_maskz_zmm(<8 x i16> %a0, i8 %mask) {
  ;CHECK-LABEL: stack_fold_pmovsxwq_maskz_zmm
  ;CHECK:       vpmovsxwq {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {z} {{.*#+}} 16-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = sext <8 x i16> %a0 to <8 x i64>
  %3 = bitcast i8 %mask to <8 x i1>
  %4 = select <8 x i1> %3, <8 x i64> %2, <8 x i64> zeroinitializer
  ret <8 x i64> %4
}

define <16 x i32> @stack_fold_pmovzxbd_zmm(<16 x i8> %a0) {
  ;CHECK-LABEL: stack_fold_pmovzxbd_zmm
  ;CHECK:       vpmovzxbd {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{.*#+}} 16-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = zext <16 x i8> %a0 to <16 x i32>
  ret <16 x i32> %2
}

define <8 x i64> @stack_fold_pmovzxbq_zmm(<16 x i8> %a0) {
  ;CHECK-LABEL: stack_fold_pmovzxbq_zmm
  ;CHECK:       vpmovzxbq {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{.*#+}} 16-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <16 x i8> %a0, <16 x i8> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %3 = zext <8 x i8> %2 to <8 x i64>
  ret <8 x i64> %3
}

define <32 x i16> @stack_fold_pmovzxbw_zmm(<32 x i8> %a0) {
  ;CHECK-LABEL: stack_fold_pmovzxbw_zmm
  ;CHECK:       vpmovzxbw {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{.*#+}} 32-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = zext <32 x i8> %a0 to <32 x i16>
  ret <32 x i16> %2
}

define <8 x i64> @stack_fold_pmovzxdq_zmm(<8 x i32> %a0) {
  ;CHECK-LABEL: stack_fold_pmovzxdq_zmm
  ;CHECK:       vpmovzxdq {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{.*#+}} 32-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = zext <8 x i32> %a0 to <8 x i64>
  ret <8 x i64> %2
}

define <16 x i32> @stack_fold_pmovzxwd_zmm(<16 x i16> %a0) {
  ;CHECK-LABEL: stack_fold_pmovzxwd_zmm
  ;CHECK:       vpmovzxwd {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{.*#+}} 32-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = zext <16 x i16> %a0 to <16 x i32>
  ret <16 x i32> %2
}

define <8 x i64> @stack_fold_pmovzxwq_zmm(<8 x i16> %a0) {
  ;CHECK-LABEL: stack_fold_pmovzxwq_zmm
  ;CHECK:       vpmovzxwq {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{.*#+}} 16-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = zext <8 x i16> %a0 to <8 x i64>
  ret <8 x i64> %2
}

define <8 x i64> @stack_fold_pmovzxwq_mask_zmm(<8 x i64> %passthru, <8 x i16> %a0, i8 %mask) {
  ;CHECK-LABEL: stack_fold_pmovzxwq_mask_zmm
  ;CHECK:       vpmovzxwq {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {{.*#+}} 16-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = zext <8 x i16> %a0 to <8 x i64>
  %3 = bitcast i8 %mask to <8 x i1>
  %4 = select <8 x i1> %3, <8 x i64> %2, <8 x i64> %passthru
  ret <8 x i64> %4
}

define <8 x i64> @stack_fold_pmovzxwq_maskz_zmm(<8 x i16> %a0, i8 %mask) {
  ;CHECK-LABEL: stack_fold_pmovzxwq_maskz_zmm
  ;CHECK:       vpmovzxwq {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {z} {{.*#+}} 16-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = zext <8 x i16> %a0 to <8 x i64>
  %3 = bitcast i8 %mask to <8 x i1>
  %4 = select <8 x i1> %3, <8 x i64> %2, <8 x i64> zeroinitializer
  ret <8 x i64> %4
}

define <64 x i8> @stack_fold_punpckhbw_zmm(<64 x i8> %a0, <64 x i8> %a1) {
  ;CHECK-LABEL: stack_fold_punpckhbw_zmm
  ;CHECK:       vpunpckhbw {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <64 x i8> %a0, <64 x i8> %a1, <64 x i32> <i32 8, i32 72, i32 9, i32 73, i32 10, i32 74, i32 11, i32 75, i32 12, i32 76, i32 13, i32 77, i32 14, i32 78, i32 15, i32 79, i32 24, i32 88, i32 25, i32 89, i32 26, i32 90, i32 27, i32 91, i32 28, i32 92, i32 29, i32 93, i32 30, i32 94, i32 31, i32 95, i32 40, i32 104, i32 41, i32 105, i32 42, i32 106, i32 43, i32 107, i32 44, i32 108, i32 45, i32 109, i32 46, i32 110, i32 47, i32 111, i32 56, i32 120, i32 57, i32 121, i32 58, i32 122, i32 59, i32 123, i32 60, i32 124, i32 61, i32 125, i32 62, i32 126, i32 63, i32 127>
  ret <64 x i8> %2
}

define <64 x i8> @stack_fold_punpckhbw_mask_zmm(<64 x i8>* %passthru, <64 x i8> %a0, <64 x i8> %a1, i64 %mask) {
  ;CHECK-LABEL: stack_fold_punpckhbw_mask_zmm
  ;CHECK:       vpunpckhbw {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <64 x i8> %a0, <64 x i8> %a1, <64 x i32> <i32 8, i32 72, i32 9, i32 73, i32 10, i32 74, i32 11, i32 75, i32 12, i32 76, i32 13, i32 77, i32 14, i32 78, i32 15, i32 79, i32 24, i32 88, i32 25, i32 89, i32 26, i32 90, i32 27, i32 91, i32 28, i32 92, i32 29, i32 93, i32 30, i32 94, i32 31, i32 95, i32 40, i32 104, i32 41, i32 105, i32 42, i32 106, i32 43, i32 107, i32 44, i32 108, i32 45, i32 109, i32 46, i32 110, i32 47, i32 111, i32 56, i32 120, i32 57, i32 121, i32 58, i32 122, i32 59, i32 123, i32 60, i32 124, i32 61, i32 125, i32 62, i32 126, i32 63, i32 127>
  %3 = bitcast i64 %mask to <64 x i1>
  ; load needed to keep the operation from being scheduled about the asm block
  %4 = load <64 x i8>, <64 x i8>* %passthru
  %5 = select <64 x i1> %3, <64 x i8> %2, <64 x i8> %4
  ret <64 x i8> %5
}

define <64 x i8> @stack_fold_punpckhbw_maskz_zmm(<64 x i8> %a0, <64 x i8> %a1, i64 %mask) {
  ;CHECK-LABEL: stack_fold_punpckhbw_maskz_zmm
  ;CHECK:       vpunpckhbw {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {z} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <64 x i8> %a0, <64 x i8> %a1, <64 x i32> <i32 8, i32 72, i32 9, i32 73, i32 10, i32 74, i32 11, i32 75, i32 12, i32 76, i32 13, i32 77, i32 14, i32 78, i32 15, i32 79, i32 24, i32 88, i32 25, i32 89, i32 26, i32 90, i32 27, i32 91, i32 28, i32 92, i32 29, i32 93, i32 30, i32 94, i32 31, i32 95, i32 40, i32 104, i32 41, i32 105, i32 42, i32 106, i32 43, i32 107, i32 44, i32 108, i32 45, i32 109, i32 46, i32 110, i32 47, i32 111, i32 56, i32 120, i32 57, i32 121, i32 58, i32 122, i32 59, i32 123, i32 60, i32 124, i32 61, i32 125, i32 62, i32 126, i32 63, i32 127>
  %3 = bitcast i64 %mask to <64 x i1>
  %4 = select <64 x i1> %3, <64 x i8> %2, <64 x i8> zeroinitializer
  ret <64 x i8> %4
}

define <64 x i8> @stack_fold_pshufb_zmm(<64 x i8> %a0, <64 x i8> %a1) {
  ;CHECK-LABEL: stack_fold_pshufb_zmm
  ;CHECK:       vpshufb {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <64 x i8> @llvm.x86.avx512.mask.pshuf.b.512(<64 x i8> %a0, <64 x i8> %a1, <64 x i8> undef, i64 -1)
  ret <64 x i8> %2
}
declare <64 x i8> @llvm.x86.avx512.mask.pshuf.b.512(<64 x i8>, <64 x i8>, <64 x i8>, i64)

define <64 x i8> @stack_fold_pshufb_zmm_mask(<64 x i8>* %passthru, <64 x i8> %a0, <64 x i8> %a1, i64 %mask) {
  ;CHECK-LABEL: stack_fold_pshufb_zmm_mask
  ;CHECK:       vpshufb {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = load <64 x i8>, <64 x i8>* %passthru
  %3 = call <64 x i8> @llvm.x86.avx512.mask.pshuf.b.512(<64 x i8> %a0, <64 x i8> %a1, <64 x i8> %2, i64 %mask)
  ret <64 x i8> %3
}

define <64 x i8> @stack_fold_pshufb_zmm_maskz(<64 x i8> %a0, <64 x i8> %a1, i64 %mask) {
  ;CHECK-LABEL: stack_fold_pshufb_zmm_maskz
  ;CHECK:       vpshufb {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {z} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <64 x i8> @llvm.x86.avx512.mask.pshuf.b.512(<64 x i8> %a0, <64 x i8> %a1, <64 x i8> zeroinitializer, i64 %mask)
  ret <64 x i8> %2
}

define <16 x i32> @stack_fold_pshufd_zmm(<16 x i32> %a0) {
  ;CHECK-LABEL: stack_fold_pshufd_zmm
  ;CHECK:       vpshufd $27, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <16 x i32> %a0, <16 x i32> undef, <16 x i32> <i32 3, i32 2, i32 1, i32 0, i32 7, i32 6, i32 5, i32 4, i32 11, i32 10, i32 9, i32 8, i32 15, i32 14, i32 13, i32 12>
  ret <16 x i32> %2
}

define <16 x i32> @stack_fold_pshufd_zmm_mask(<16 x i32> %passthru, <16 x i32> %a0, i16 %mask) {
  ;CHECK-LABEL: stack_fold_pshufd_zmm_mask
  ;CHECK:       vpshufd $27, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <16 x i32> %a0, <16 x i32> undef, <16 x i32> <i32 3, i32 2, i32 1, i32 0, i32 7, i32 6, i32 5, i32 4, i32 11, i32 10, i32 9, i32 8, i32 15, i32 14, i32 13, i32 12>
  %3 = bitcast i16 %mask to <16 x i1>
  %4 = select <16 x i1> %3, <16 x i32> %2, <16 x i32> %passthru
  ret <16 x i32> %4
}

define <16 x i32> @stack_fold_pshufd_zmm_maskz(<16 x i32> %a0, i16 %mask) {
  ;CHECK-LABEL: stack_fold_pshufd_zmm_maskz
  ;CHECK:       vpshufd $27, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <16 x i32> %a0, <16 x i32> undef, <16 x i32> <i32 3, i32 2, i32 1, i32 0, i32 7, i32 6, i32 5, i32 4, i32 11, i32 10, i32 9, i32 8, i32 15, i32 14, i32 13, i32 12>
  %3 = bitcast i16 %mask to <16 x i1>
  %4 = select <16 x i1> %3, <16 x i32> %2, <16 x i32> zeroinitializer
  ret <16 x i32> %4
}

define <32 x i16> @stack_fold_pshufhw_zmm(<32 x i16> %a0) {
  ;CHECK-LABEL: stack_fold_pshufhw_zmm
  ;CHECK:       vpshufhw $27, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <32 x i16> %a0, <32 x i16> undef, <32 x i32> <i32 0, i32 1, i32 2, i32 3, i32 7, i32 6, i32 5, i32 4, i32 8, i32 9, i32 10, i32 11, i32 15, i32 14, i32 13, i32 12, i32 16, i32 17, i32 18, i32 19, i32 23, i32 22, i32 21, i32 20, i32 24, i32 25, i32 26, i32 27, i32 31, i32 30, i32 29, i32 28>
  ret <32 x i16> %2
}

define <32 x i16> @stack_fold_pshufhw_zmm_mask(<32 x i16> %passthru, <32 x i16> %a0, i32 %mask) {
  ;CHECK-LABEL: stack_fold_pshufhw_zmm_mask
  ;CHECK:       vpshufhw $27, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <32 x i16> %a0, <32 x i16> undef, <32 x i32> <i32 0, i32 1, i32 2, i32 3, i32 7, i32 6, i32 5, i32 4, i32 8, i32 9, i32 10, i32 11, i32 15, i32 14, i32 13, i32 12, i32 16, i32 17, i32 18, i32 19, i32 23, i32 22, i32 21, i32 20, i32 24, i32 25, i32 26, i32 27, i32 31, i32 30, i32 29, i32 28>
  %3 = bitcast i32 %mask to <32 x i1>
  %4 = select <32 x i1> %3, <32 x i16> %2, <32 x i16> %passthru
  ret <32 x i16> %4
}

define <32 x i16> @stack_fold_pshufhw_zmm_maskz(<32 x i16> %a0, i32 %mask) {
  ;CHECK-LABEL: stack_fold_pshufhw_zmm_maskz
  ;CHECK:       vpshufhw $27, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <32 x i16> %a0, <32 x i16> undef, <32 x i32> <i32 0, i32 1, i32 2, i32 3, i32 7, i32 6, i32 5, i32 4, i32 8, i32 9, i32 10, i32 11, i32 15, i32 14, i32 13, i32 12, i32 16, i32 17, i32 18, i32 19, i32 23, i32 22, i32 21, i32 20, i32 24, i32 25, i32 26, i32 27, i32 31, i32 30, i32 29, i32 28>
  %3 = bitcast i32 %mask to <32 x i1>
  %4 = select <32 x i1> %3, <32 x i16> %2, <32 x i16> zeroinitializer
  ret <32 x i16> %4
}

define <32 x i16> @stack_fold_pshuflw_zmm(<32 x i16> %a0) {
  ;CHECK-LABEL: stack_fold_pshuflw_zmm
  ;CHECK:       vpshuflw $27, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <32 x i16> %a0, <32 x i16> undef, <32 x i32> <i32 3, i32 2, i32 1, i32 0, i32 4, i32 5, i32 6, i32 7, i32 11, i32 10, i32 9, i32 8, i32 12, i32 13, i32 14, i32 15, i32 19, i32 18, i32 17, i32 16, i32 20, i32 21, i32 22, i32 23, i32 27, i32 26, i32 25, i32 24, i32 28, i32 29, i32 30, i32 31>
  ret <32 x i16> %2
}

define <32 x i16> @stack_fold_pshuflw_zmm_mask(<32 x i16> %passthru, <32 x i16> %a0, i32 %mask) {
  ;CHECK-LABEL: stack_fold_pshuflw_zmm_mask
  ;CHECK:       vpshuflw $27, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <32 x i16> %a0, <32 x i16> undef, <32 x i32> <i32 3, i32 2, i32 1, i32 0, i32 4, i32 5, i32 6, i32 7, i32 11, i32 10, i32 9, i32 8, i32 12, i32 13, i32 14, i32 15, i32 19, i32 18, i32 17, i32 16, i32 20, i32 21, i32 22, i32 23, i32 27, i32 26, i32 25, i32 24, i32 28, i32 29, i32 30, i32 31>
  %3 = bitcast i32 %mask to <32 x i1>
  %4 = select <32 x i1> %3, <32 x i16> %2, <32 x i16> %passthru
  ret <32 x i16> %4
}

define <32 x i16> @stack_fold_pshuflw_zmm_maskz(<32 x i16> %a0, i32 %mask) {
  ;CHECK-LABEL: stack_fold_pshuflw_zmm_maskz
  ;CHECK:       vpshuflw $27, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {z} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <32 x i16> %a0, <32 x i16> undef, <32 x i32> <i32 3, i32 2, i32 1, i32 0, i32 4, i32 5, i32 6, i32 7, i32 11, i32 10, i32 9, i32 8, i32 12, i32 13, i32 14, i32 15, i32 19, i32 18, i32 17, i32 16, i32 20, i32 21, i32 22, i32 23, i32 27, i32 26, i32 25, i32 24, i32 28, i32 29, i32 30, i32 31>
  %3 = bitcast i32 %mask to <32 x i1>
  %4 = select <32 x i1> %3, <32 x i16> %2, <32 x i16> zeroinitializer
  ret <32 x i16> %4
}

define <32 x i16> @stack_fold_pmaddubsw_zmm(<64 x i8> %a0, <64 x i8> %a1) {
  ;CHECK-LABEL: stack_fold_pmaddubsw_zmm
  ;CHECK:       vpmaddubsw {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <32 x i16> @llvm.x86.avx512.mask.pmaddubs.w.512(<64 x i8> %a0, <64 x i8> %a1, <32 x i16> undef, i32 -1)
  ret <32 x i16> %2
}
declare <32 x i16> @llvm.x86.avx512.mask.pmaddubs.w.512(<64 x i8>, <64 x i8>, <32 x i16>, i32) nounwind readnone

define <32 x i16> @stack_fold_pmaddubsw_zmm_mask(<32 x i16>* %passthru, <64 x i8> %a0, <64 x i8> %a1, i32 %mask) {
  ;CHECK-LABEL: stack_fold_pmaddubsw_zmm_mask
  ;CHECK:       vpmaddubsw {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <32 x i16> @llvm.x86.avx512.mask.pmaddubs.w.512(<64 x i8> %a0, <64 x i8> %a1, <32 x i16> undef, i32 -1)
  %3 = bitcast i32 %mask to <32 x i1>
  ; load needed to keep the operation from being scheduled about the asm block
  %4 = load <32 x i16>, <32 x i16>* %passthru
  %5 = select <32 x i1> %3, <32 x i16> %2, <32 x i16> %4
  ret <32 x i16> %5
}

define <32 x i16> @stack_fold_pmaddubsw_zmm_maskz(<64 x i8> %a0, <64 x i8> %a1, i32 %mask) {
  ;CHECK-LABEL: stack_fold_pmaddubsw_zmm_maskz
  ;CHECK:       vpmaddubsw {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {z} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <32 x i16> @llvm.x86.avx512.mask.pmaddubs.w.512(<64 x i8> %a0, <64 x i8> %a1, <32 x i16> undef, i32 -1)
  %3 = bitcast i32 %mask to <32 x i1>
  %4 = select <32 x i1> %3, <32 x i16> %2, <32 x i16> zeroinitializer
  ret <32 x i16> %4
}

define <16 x i32> @stack_fold_pmaddwd_zmm(<32 x i16> %a0, <32 x i16> %a1) {
  ;CHECK-LABEL: stack_fold_pmaddwd_zmm
  ;CHECK:       vpmaddwd {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <16 x i32> @llvm.x86.avx512.mask.pmaddw.d.512(<32 x i16> %a0, <32 x i16> %a1, <16 x i32> undef, i16 -1)
  ret <16 x i32> %2
}
declare <16 x i32> @llvm.x86.avx512.mask.pmaddw.d.512(<32 x i16>, <32 x i16>, <16 x i32>, i16) nounwind readnone

define <16 x i32> @stack_fold_pmaddwd_zmm_mask(<16 x i32>* %passthru, <32 x i16> %a0, <32 x i16> %a1, i16 %mask) {
  ;CHECK-LABEL: stack_fold_pmaddwd_zmm_mask
  ;CHECK:       vpmaddwd {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <16 x i32> @llvm.x86.avx512.mask.pmaddw.d.512(<32 x i16> %a0, <32 x i16> %a1, <16 x i32> undef, i16 -1)
  %3 = bitcast i16 %mask to <16 x i1>
  ; load needed to keep the operation from being scheduled about the asm block
  %4 = load <16 x i32>, <16 x i32>* %passthru
  %5 = select <16 x i1> %3, <16 x i32> %2, <16 x i32> %4
  ret <16 x i32> %5
}

define <16 x i32> @stack_fold_pmaddwd_zmm_maskz(<16 x i32>* %passthru, <32 x i16> %a0, <32 x i16> %a1, i16 %mask) {
  ;CHECK-LABEL: stack_fold_pmaddwd_zmm_maskz
  ;CHECK:       vpmaddwd {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {z} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <16 x i32> @llvm.x86.avx512.mask.pmaddw.d.512(<32 x i16> %a0, <32 x i16> %a1, <16 x i32> undef, i16 -1)
  %3 = bitcast i16 %mask to <16 x i1>
  %4 = select <16 x i1> %3, <16 x i32> %2, <16 x i32> zeroinitializer
  ret <16 x i32> %4
}

define <16 x i32> @stack_fold_permd(<16 x i32> %a0, <16 x i32> %a1) {
  ;CHECK-LABEL: stack_fold_permd
  ;CHECK:   vpermd {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <16 x i32> @llvm.x86.avx512.mask.permvar.si.512(<16 x i32> %a1, <16 x i32> %a0, <16 x i32> undef, i16 -1)
  ret <16 x i32> %2
}
declare <16 x i32> @llvm.x86.avx512.mask.permvar.si.512(<16 x i32>, <16 x i32>, <16 x i32>, i16) nounwind readonly

define <8 x i64> @stack_fold_permq(<8 x i64> %a0) {
  ;CHECK-LABEL: stack_fold_permq
  ;CHECK:   vpermq $235, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <8 x i64> %a0, <8 x i64> undef, <8 x i32> <i32 3, i32 2, i32 2, i32 3, i32 7, i32 6, i32 6, i32 7>
  ; add forces execution domain
  %3 = add <8 x i64> %2, <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  ret <8 x i64> %3
}

define <8 x i64> @stack_fold_permq_mask(<8 x i64>* %passthru, <8 x i64> %a0, i8 %mask) {
  ;CHECK-LABEL: stack_fold_permq_mask
  ;CHECK:   vpermq $235, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <8 x i64> %a0, <8 x i64> undef, <8 x i32> <i32 3, i32 2, i32 2, i32 3, i32 7, i32 6, i32 6, i32 7>
  %3 = bitcast i8 %mask to <8 x i1>
  ; load needed to keep the operation from being scheduled above the asm block
  %4 = load <8 x i64>, <8 x i64>* %passthru
  %5 = select <8 x i1> %3, <8 x i64> %2, <8 x i64> %4
  ; add forces execution domain
  %6 = add <8 x i64> %5, <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  ret <8 x i64> %6
}

define <8 x i64> @stack_fold_permq_maskz(<8 x i64>* %passthru, <8 x i64> %a0, i8 %mask) {
  ;CHECK-LABEL: stack_fold_permq_maskz
  ;CHECK:   vpermq $235, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {z} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <8 x i64> %a0, <8 x i64> undef, <8 x i32> <i32 3, i32 2, i32 2, i32 3, i32 7, i32 6, i32 6, i32 7>
  %3 = bitcast i8 %mask to <8 x i1>
  %4 = select <8 x i1> %3, <8 x i64> %2, <8 x i64> zeroinitializer
  ret <8 x i64> %4
}

define <8 x i64> @stack_fold_permqvar(<8 x i64> %a0, <8 x i64> %a1) {
  ;CHECK-LABEL: stack_fold_permqvar
  ;CHECK:   vpermq {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <8 x i64> @llvm.x86.avx512.mask.permvar.di.512(<8 x i64> %a1, <8 x i64> %a0, <8 x i64> undef, i8 -1)
  ; add forces execution domain
  %3 = add <8 x i64> %2, <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  ret <8 x i64> %3
}
declare <8 x i64> @llvm.x86.avx512.mask.permvar.di.512(<8 x i64>, <8 x i64>, <8 x i64>, i8) nounwind readonly

define <8 x i64> @stack_fold_permqvar_mask(<8 x i64>* %passthru, <8 x i64> %a0, <8 x i64> %a1, i8 %mask) {
  ;CHECK-LABEL: stack_fold_permqvar_mask
  ;CHECK:   vpermq {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <8 x i64> @llvm.x86.avx512.mask.permvar.di.512(<8 x i64> %a1, <8 x i64> %a0, <8 x i64> undef, i8 -1)
  %3 = bitcast i8 %mask to <8 x i1>
  ; load needed to keep the operation from being scheduled above the asm block
  %4 = load <8 x i64>, <8 x i64>* %passthru
  %5 = select <8 x i1> %3, <8 x i64> %2, <8 x i64> %4
  ; add forces execution domain
  %6 = add <8 x i64> %5, <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  ret <8 x i64> %6
}

define <32 x i16> @stack_fold_permwvar(<32 x i16> %a0, <32 x i16> %a1) {
  ;CHECK-LABEL: stack_fold_permwvar
  ;CHECK:   vpermw {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <32 x i16> @llvm.x86.avx512.mask.permvar.hi.512(<32 x i16> %a1, <32 x i16> %a0, <32 x i16> undef, i32 -1)
  ret <32 x i16> %2
}
declare <32 x i16> @llvm.x86.avx512.mask.permvar.hi.512(<32 x i16>, <32 x i16>, <32 x i16>, i32) nounwind readonly

define <32 x i16> @stack_fold_permwvar_mask(<32 x i16>* %passthru, <32 x i16> %a0, <32 x i16> %a1, i32 %mask) {
  ;CHECK-LABEL: stack_fold_permwvar_mask
  ;CHECK:   vpermw {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <32 x i16> @llvm.x86.avx512.mask.permvar.hi.512(<32 x i16> %a1, <32 x i16> %a0, <32 x i16> undef, i32 -1)
  %3 = bitcast i32 %mask to <32 x i1>
  ; load needed to keep the operation from being scheduled above the asm block
  %4 = load <32 x i16>, <32 x i16>* %passthru
  %5 = select <32 x i1> %3, <32 x i16> %2, <32 x i16> %4
  ret <32 x i16> %5
}

define <32 x i16> @stack_fold_permwvar_maskz(<32 x i16> %a0, <32 x i16> %a1, i32 %mask) {
  ;CHECK-LABEL: stack_fold_permwvar_maskz
  ;CHECK:   vpermw {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {z} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <32 x i16> @llvm.x86.avx512.mask.permvar.hi.512(<32 x i16> %a1, <32 x i16> %a0, <32 x i16> undef, i32 -1)
  %3 = bitcast i32 %mask to <32 x i1>
  %4 = select <32 x i1> %3, <32 x i16> %2, <32 x i16> zeroinitializer
  ret <32 x i16> %4
}

define <64 x i8> @stack_fold_permbvar(<64 x i8> %a0, <64 x i8> %a1) {
  ;CHECK-LABEL: stack_fold_permbvar
  ;CHECK:   vpermb {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <64 x i8> @llvm.x86.avx512.mask.permvar.qi.512(<64 x i8> %a1, <64 x i8> %a0, <64 x i8> undef, i64 -1)
  ret <64 x i8> %2
}
declare <64 x i8> @llvm.x86.avx512.mask.permvar.qi.512(<64 x i8>, <64 x i8>, <64 x i8>, i64) nounwind readonly

define <64 x i8> @stack_fold_permbvar_mask(<64 x i8>* %passthru, <64 x i8> %a0, <64 x i8> %a1, i64 %mask) {
  ;CHECK-LABEL: stack_fold_permbvar_mask
  ;CHECK:   vpermb {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <64 x i8> @llvm.x86.avx512.mask.permvar.qi.512(<64 x i8> %a1, <64 x i8> %a0, <64 x i8> undef, i64 -1)
  %3 = bitcast i64 %mask to <64 x i1>
  ; load needed to keep the operation from being scheduled above the asm block
  %4 = load <64 x i8>, <64 x i8>* %passthru
  %5 = select <64 x i1> %3, <64 x i8> %2, <64 x i8> %4
  ret <64 x i8> %5
}

define <64 x i8> @stack_fold_permbvar_maskz(<64 x i8> %a0, <64 x i8> %a1, i64 %mask) {
  ;CHECK-LABEL: stack_fold_permbvar_maskz
  ;CHECK:   vpermb {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {z} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = call <64 x i8> @llvm.x86.avx512.mask.permvar.qi.512(<64 x i8> %a1, <64 x i8> %a0, <64 x i8> undef, i64 -1)
  %3 = bitcast i64 %mask to <64 x i1>
  %4 = select <64 x i1> %3, <64 x i8> %2, <64 x i8> zeroinitializer
  ret <64 x i8> %4
}

define <8 x i64> @stack_fold_valignq(<8 x i64> %a, <8 x i64> %b) {
  ;CHECK-LABEL: stack_fold_valignq
  ;CHECK:   valignq $1, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <8 x i64> %a, <8 x i64> %b, <8 x i32> <i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8>
  ret <8 x i64> %2
}

define <8 x i64> @stack_fold_valignq_mask(<8 x i64> %a, <8 x i64> %b, <8 x i64>* %passthru, i8 %mask) {
  ;CHECK-LABEL: stack_fold_valignq_mask
  ;CHECK:   valignq $1, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <8 x i64> %a, <8 x i64> %b, <8 x i32> <i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8>
  %3 = bitcast i8 %mask to <8 x i1>
  %4 = load <8 x i64>, <8 x i64>* %passthru
  %5 = select <8 x i1> %3, <8 x i64> %2, <8 x i64> %4
  ret <8 x i64> %5
}

define <8 x i64> @stack_fold_valignq_maskz(<8 x i64> %a, <8 x i64> %b, i8 %mask) {
  ;CHECK-LABEL: stack_fold_valignq_maskz
  ;CHECK:   valignq $1, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {z} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <8 x i64> %a, <8 x i64> %b, <8 x i32> <i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8>
  %3 = bitcast i8 %mask to <8 x i1>
  %4 = select <8 x i1> %3, <8 x i64> %2, <8 x i64> zeroinitializer
  ret <8 x i64> %4
}

define <16 x i32> @stack_fold_valignd(<16 x i32> %a, <16 x i32> %b) {
  ;CHECK-LABEL: stack_fold_valignd
  ;CHECK:   valignd $1, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <16 x i32> %a, <16 x i32> %b, <16 x i32><i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16>
  ret <16 x i32> %2
}

define <16 x i32> @stack_fold_valignd_mask(<16 x i32> %a, <16 x i32> %b, <16 x i32>* %passthru, i16 %mask) {
  ;CHECK-LABEL: stack_fold_valignd_mask
  ;CHECK:   valignd $1, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <16 x i32> %a, <16 x i32> %b, <16 x i32><i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16>
  %3 = bitcast i16 %mask to <16 x i1>
  %4 = load <16 x i32>, <16 x i32>* %passthru
  %5 = select <16 x i1> %3, <16 x i32> %2, <16 x i32> %4
  ret <16 x i32> %5
}

define <16 x i32> @stack_fold_valignd_maskz(<16 x i32> %a, <16 x i32> %b, i16 %mask) {
  ;CHECK-LABEL: stack_fold_valignd_maskz
  ;CHECK:   valignd $1, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {z} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <16 x i32> %a, <16 x i32> %b, <16 x i32><i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16>
  %3 = bitcast i16 %mask to <16 x i1>
  %4 = select <16 x i1> %3, <16 x i32> %2, <16 x i32> zeroinitializer
  ret <16 x i32> %4
}

define <64 x i8> @stack_fold_palignr(<64 x i8> %a0, <64 x i8> %a1) {
  ;CHECK-LABEL: stack_fold_palignr
  ;CHECK:       vpalignr $1, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <64 x i8> %a1, <64 x i8> %a0, <64 x i32> <i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 64, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 80, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39, i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 96, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55, i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63, i32 112>
  ret <64 x i8> %2
}

define <64 x i8> @stack_fold_palignr_mask(<64 x i8> %a0, <64 x i8> %a1, <64 x i8>* %passthru, i64 %mask) {
  ;CHECK-LABEL: stack_fold_palignr_mask
  ;CHECK:       vpalignr $1, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <64 x i8> %a1, <64 x i8> %a0, <64 x i32> <i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 64, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 80, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39, i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 96, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55, i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63, i32 112>
  %3 = bitcast i64 %mask to <64 x i1>
  %4 = load <64 x i8>, <64 x i8>* %passthru
  %5 = select <64 x i1> %3, <64 x i8> %2, <64 x i8> %4
  ret <64 x i8> %5
}

define <64 x i8> @stack_fold_palignr_maskz(<64 x i8> %a0, <64 x i8> %a1, i64 %mask) {
  ;CHECK-LABEL: stack_fold_palignr_maskz
  ;CHECK:       vpalignr $1, {{-?[0-9]*}}(%rsp), {{%zmm[0-9][0-9]*}}, {{%zmm[0-9][0-9]*}} {{{%k[0-7]}}} {z} {{.*#+}} 64-byte Folded Reload
  %1 = tail call <2 x i64> asm sideeffect "nop", "=x,~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15},~{xmm16},~{xmm17},~{xmm18},~{xmm19},~{xmm20},~{xmm21},~{xmm22},~{xmm23},~{xmm24},~{xmm25},~{xmm26},~{xmm27},~{xmm28},~{xmm29},~{xmm30},~{xmm31},~{flags}"()
  %2 = shufflevector <64 x i8> %a1, <64 x i8> %a0, <64 x i32> <i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 64, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 80, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39, i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 96, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55, i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63, i32 112>
  %3 = bitcast i64 %mask to <64 x i1>
  %4 = select <64 x i1> %3, <64 x i8> %2, <64 x i8> zeroinitializer
  ret <64 x i8> %4
}

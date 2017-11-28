/* Copyright (C) 2007-2017 Free Software Foundation, Inc.

   This file is part of the GNU opcodes library.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include <stdio.h>
#include <errno.h>
#include "getopt.h"
#include "libiberty.h"
#include "hashtab.h"
#include "safe-ctype.h"

#include "i386-opc.h"

#include <libintl.h>
#define _(String) gettext (String)

static const char *program_name = NULL;
static int debug = 0;

typedef struct initializer
{
  const char *name;
  const char *init;
} initializer;

static initializer cpu_flag_init[] =
{
  { "CPU_UNKNOWN_FLAGS",
    "~(CpuL1OM|CpuK1OM)" },
  { "CPU_GENERIC32_FLAGS",
    "Cpu186|Cpu286|Cpu386" },
  { "CPU_GENERIC64_FLAGS",
    "CPU_PENTIUMPRO_FLAGS|CpuClflush|CpuSYSCALL|CPU_MMX_FLAGS|CPU_SSE2_FLAGS|CpuLM" },
  { "CPU_NONE_FLAGS",
   "0" },
  { "CPU_I186_FLAGS",
    "Cpu186" },
  { "CPU_I286_FLAGS",
    "CPU_I186_FLAGS|Cpu286" },
  { "CPU_I386_FLAGS",
    "CPU_I286_FLAGS|Cpu386" },
  { "CPU_I486_FLAGS",
    "CPU_I386_FLAGS|Cpu486" },
  { "CPU_I586_FLAGS",
    "CPU_I486_FLAGS|CPU_387_FLAGS|Cpu586" },
  { "CPU_I686_FLAGS",
    "CPU_I586_FLAGS|Cpu686|Cpu687" },
  { "CPU_PENTIUMPRO_FLAGS",
    "CPU_I686_FLAGS|CpuNop" },
  { "CPU_P2_FLAGS",
    "CPU_PENTIUMPRO_FLAGS|CPU_MMX_FLAGS" },
  { "CPU_P3_FLAGS",
    "CPU_P2_FLAGS|CPU_SSE_FLAGS" },
  { "CPU_P4_FLAGS",
    "CPU_P3_FLAGS|CpuClflush|CPU_SSE2_FLAGS" },
  { "CPU_NOCONA_FLAGS",
    "CPU_GENERIC64_FLAGS|CpuFISTTP|CPU_SSE3_FLAGS|CpuCX16" },
  { "CPU_CORE_FLAGS",
    "CPU_P4_FLAGS|CpuFISTTP|CPU_SSE3_FLAGS|CpuCX16" },
  { "CPU_CORE2_FLAGS",
    "CPU_NOCONA_FLAGS|CPU_SSSE3_FLAGS" },
  { "CPU_COREI7_FLAGS",
    "CPU_CORE2_FLAGS|CPU_SSE4_2_FLAGS|CpuRdtscp" },
  { "CPU_K6_FLAGS",
    "Cpu186|Cpu286|Cpu386|Cpu486|Cpu586|CpuSYSCALL|Cpu387|CPU_MMX_FLAGS" },
  { "CPU_K6_2_FLAGS",
    "CPU_K6_FLAGS|Cpu3dnow" },
  { "CPU_ATHLON_FLAGS",
    "CPU_K6_2_FLAGS|Cpu686|Cpu687|CpuNop|Cpu3dnowA" },
  { "CPU_K8_FLAGS",
    "CPU_ATHLON_FLAGS|CpuRdtscp|CPU_SSE2_FLAGS|CpuLM" },
  { "CPU_AMDFAM10_FLAGS",
    "CPU_K8_FLAGS|CpuFISTTP|CPU_SSE4A_FLAGS|CpuABM" },
  { "CPU_BDVER1_FLAGS",
    "CPU_GENERIC64_FLAGS|CpuFISTTP|CpuRdtscp|CpuCX16|CPU_SSE4_2_FLAGS|CpuSSE4A|CpuABM|CpuFMA4|CpuXOP|CpuLWP|CpuSVME|CpuXsave|CpuAES|CpuAVX|CpuPCLMUL|CpuLZCNT|CpuPRFCHW" },
  { "CPU_BDVER2_FLAGS",
    "CPU_BDVER1_FLAGS|CpuFMA|CpuBMI|CpuTBM|CpuF16C" },
  { "CPU_BDVER3_FLAGS",
    "CPU_BDVER2_FLAGS|CpuXsaveopt|CpuFSGSBase" },
  { "CPU_BDVER4_FLAGS",
    "CPU_BDVER3_FLAGS|CpuAVX2|CpuMovbe|CpuBMI2|CpuRdRnd|CpuMWAITX" },
  { "CPU_ZNVER1_FLAGS",
    "CPU_GENERIC64_FLAGS|CpuFISTTP|CpuRdtscp|CpuCX16|CPU_SSE4_2_FLAGS|CpuSSE4A|CpuABM|CpuSVME|CpuXsave|CpuAES|CpuAVX|CpuPCLMUL|CpuLZCNT|CpuPRFCHW|CpuFMA|CpuBMI|CpuF16C|CpuXsaveopt|CpuFSGSBase|CpuAVX2|CpuMovbe|CpuBMI2|CpuRdRnd|CpuADX|CpuRdSeed|CpuSMAP|CpuSHA|CpuXSAVEC|CpuXSAVES|CpuClflushOpt|CpuCLZERO|CpuMWAITX" },
  { "CPU_BTVER1_FLAGS",
    "CPU_GENERIC64_FLAGS|CpuFISTTP|CpuCX16|CpuRdtscp|CPU_SSSE3_FLAGS|CpuSSE4A|CpuABM|CpuPRFCHW|CpuCX16|CpuClflush|CpuFISTTP|CpuSVME|CpuLZCNT" },
  { "CPU_BTVER2_FLAGS",
    "CPU_BTVER1_FLAGS|CPU_SSE4_2_FLAGS|CpuBMI|CpuF16C|CpuAES|CpuPCLMUL|CpuAVX|CpuMovbe|CpuXsave|CpuXsaveopt|CpuPRFCHW" },
  { "CPU_8087_FLAGS",
    "Cpu8087" },
  { "CPU_287_FLAGS",
    "CPU_8087_FLAGS|Cpu287" },
  { "CPU_387_FLAGS",
    "CPU_287_FLAGS|Cpu387" },
  { "CPU_687_FLAGS",
    "CPU_387_FLAGS|Cpu687" },
  { "CPU_CLFLUSH_FLAGS",
    "CpuClflush" },
  { "CPU_NOP_FLAGS",
    "CpuNop" },
  { "CPU_SYSCALL_FLAGS",
    "CpuSYSCALL" },
  { "CPU_MMX_FLAGS",
    "CpuRegMMX|CpuMMX" },
  { "CPU_SSE_FLAGS",
    "CpuRegXMM|CpuSSE" },
  { "CPU_SSE2_FLAGS",
    "CPU_SSE_FLAGS|CpuSSE2" },
  { "CPU_SSE3_FLAGS",
    "CPU_SSE2_FLAGS|CpuSSE3" },
  { "CPU_SSSE3_FLAGS",
    "CPU_SSE3_FLAGS|CpuSSSE3" },
  { "CPU_SSE4_1_FLAGS",
    "CPU_SSSE3_FLAGS|CpuSSE4_1" },
  { "CPU_SSE4_2_FLAGS",
    "CPU_SSE4_1_FLAGS|CpuSSE4_2" },
  { "CPU_VMX_FLAGS",
    "CpuVMX" },
  { "CPU_SMX_FLAGS",
    "CpuSMX" },
  { "CPU_XSAVE_FLAGS",
    "CpuXsave" },
  { "CPU_XSAVEOPT_FLAGS",
    "CPU_XSAVE_FLAGS|CpuXsaveopt" },
  { "CPU_AES_FLAGS",
    "CPU_SSE2_FLAGS|CpuAES" },
  { "CPU_PCLMUL_FLAGS",
    "CPU_SSE2_FLAGS|CpuPCLMUL" },
  { "CPU_FMA_FLAGS",
    "CPU_AVX_FLAGS|CpuFMA" },
  { "CPU_FMA4_FLAGS",
    "CPU_AVX_FLAGS|CpuFMA4" },
  { "CPU_XOP_FLAGS",
    "CPU_SSE4A_FLAGS|CPU_FMA4_FLAGS|CpuXOP" },
  { "CPU_LWP_FLAGS",
    "CpuLWP" },
  { "CPU_BMI_FLAGS",
    "CpuBMI" },
  { "CPU_TBM_FLAGS",
    "CpuTBM" },
  { "CPU_MOVBE_FLAGS",
    "CpuMovbe" },
  { "CPU_CX16_FLAGS",
    "CpuCX16" },
  { "CPU_RDTSCP_FLAGS",
    "CpuRdtscp" },
  { "CPU_EPT_FLAGS",
    "CpuEPT" },
  { "CPU_FSGSBASE_FLAGS",
    "CpuFSGSBase" },
  { "CPU_RDRND_FLAGS",
    "CpuRdRnd" },
  { "CPU_F16C_FLAGS",
    "CPU_AVX_FLAGS|CpuF16C" },
  { "CPU_BMI2_FLAGS",
    "CpuBMI2" },
  { "CPU_LZCNT_FLAGS",
    "CpuLZCNT" },
  { "CPU_HLE_FLAGS",
    "CpuHLE" },
  { "CPU_RTM_FLAGS",
    "CpuRTM" },
  { "CPU_INVPCID_FLAGS",
    "CpuINVPCID" },
  { "CPU_VMFUNC_FLAGS",
    "CpuVMFUNC" },
  { "CPU_3DNOW_FLAGS",
    "CPU_MMX_FLAGS|Cpu3dnow" },
  { "CPU_3DNOWA_FLAGS",
    "CPU_3DNOW_FLAGS|Cpu3dnowA" },
  { "CPU_PADLOCK_FLAGS",
    "CpuPadLock" },
  { "CPU_SVME_FLAGS",
    "CpuSVME" },
  { "CPU_SSE4A_FLAGS",
    "CPU_SSE3_FLAGS|CpuSSE4a" },
  { "CPU_ABM_FLAGS",
    "CpuABM" },
  { "CPU_AVX_FLAGS",
    "CPU_SSE4_2_FLAGS|CpuRegYMM|CpuAVX" },
  { "CPU_AVX2_FLAGS",
    "CPU_AVX_FLAGS|CpuAVX2" },
  /* Don't use CPU_AVX2_FLAGS on CPU_AVX512F_FLAGS since AVX512F doesn't
     support YMM registers.  */
  { "CPU_AVX512F_FLAGS",
    "CpuVREX|CPU_SSE4_2_FLAGS|CpuRegZMM|CpuRegMask|CpuAVX|CpuAVX2|CpuAVX512F" },
  { "CPU_AVX512CD_FLAGS",
    "CPU_AVX512F_FLAGS|CpuAVX512CD" },
  { "CPU_AVX512ER_FLAGS",
    "CPU_AVX512F_FLAGS|CpuAVX512ER" },
  { "CPU_AVX512PF_FLAGS",
    "CPU_AVX512F_FLAGS|CpuAVX512PF" },
  { "CPU_AVX512DQ_FLAGS",
    "CPU_AVX512F_FLAGS|CpuAVX512DQ" },
  { "CPU_AVX512BW_FLAGS",
    "CPU_AVX512F_FLAGS|CpuAVX512BW" },
  { "CPU_AVX512VL_FLAGS",
  /* Use CPU_AVX2_FLAGS on CPU_AVX512VL_FLAGS since AVX512VL supports YMM
     registers.  */
    "CPU_AVX512F_FLAGS|CPU_AVX2_FLAGS|CpuAVX512VL" },
  { "CPU_AVX512IFMA_FLAGS",
    "CPU_AVX512F_FLAGS|CpuAVX512IFMA" },
  { "CPU_AVX512VBMI_FLAGS",
    "CPU_AVX512F_FLAGS|CpuAVX512VBMI" },
  { "CPU_AVX512_4FMAPS_FLAGS",
    "CPU_AVX512F_FLAGS|CpuAVX512_4FMAPS" },
  { "CPU_AVX512_4VNNIW_FLAGS",
    "CPU_AVX512F_FLAGS|CpuAVX512_4VNNIW" },
  { "CPU_AVX512_VPOPCNTDQ_FLAGS",
    "CPU_AVX512F_FLAGS|CpuAVX512_VPOPCNTDQ" },
  { "CPU_L1OM_FLAGS",
    "unknown" },
  { "CPU_K1OM_FLAGS",
    "unknown" },
  { "CPU_IAMCU_FLAGS",
    "Cpu186|Cpu286|Cpu386|Cpu486|Cpu586" },
  { "CPU_ADX_FLAGS",
    "CpuADX" },
  { "CPU_RDSEED_FLAGS",
    "CpuRdSeed" },
  { "CPU_PRFCHW_FLAGS",
    "CpuPRFCHW" },
  { "CPU_SMAP_FLAGS",
    "CpuSMAP" },
  { "CPU_MPX_FLAGS",
    "CpuMPX" },
  { "CPU_SHA_FLAGS",
    "CPU_SSE2_FLAGS|CpuSHA" },
  { "CPU_CLFLUSHOPT_FLAGS",
    "CpuClflushOpt" },
  { "CPU_XSAVES_FLAGS",
    "CPU_XSAVE_FLAGS|CpuXSAVES" },
  { "CPU_XSAVEC_FLAGS",
    "CPU_XSAVE_FLAGS|CpuXSAVEC" },
  { "CPU_PREFETCHWT1_FLAGS",
    "CpuPREFETCHWT1" },
  { "CPU_SE1_FLAGS",
    "CpuSE1" },
  { "CPU_CLWB_FLAGS",
    "CpuCLWB" },
  { "CPU_CLZERO_FLAGS",
    "CpuCLZERO" },
  { "CPU_MWAITX_FLAGS",
    "CpuMWAITX" },
  { "CPU_OSPKE_FLAGS",
    "CpuOSPKE" },
  { "CPU_RDPID_FLAGS",
    "CpuRDPID" },
  { "CPU_PTWRITE_FLAGS",
    "CpuPTWRITE" },
  { "CPU_CET_FLAGS",
    "CpuCET" },
  { "CPU_ANY_X87_FLAGS",
    "CPU_ANY_287_FLAGS|Cpu8087" },
  { "CPU_ANY_287_FLAGS",
    "CPU_ANY_387_FLAGS|Cpu287" },
  { "CPU_ANY_387_FLAGS",
    "CPU_ANY_687_FLAGS|Cpu387" },
  { "CPU_ANY_687_FLAGS",
    "Cpu687|CpuFISTTP" },
  { "CPU_ANY_MMX_FLAGS",
    "CPU_3DNOWA_FLAGS" },
  { "CPU_ANY_SSE_FLAGS",
    "CPU_ANY_SSE2_FLAGS|CpuSSE|CpuSSE4a" },
  { "CPU_ANY_SSE2_FLAGS",
    "CPU_ANY_SSE3_FLAGS|CpuSSE2" },
  { "CPU_ANY_SSE3_FLAGS",
    "CPU_ANY_SSSE3_FLAGS|CpuSSE3" },
  { "CPU_ANY_SSSE3_FLAGS",
    "CPU_ANY_SSE4_1_FLAGS|CpuSSSE3" },
  { "CPU_ANY_SSE4_1_FLAGS",
    "CPU_ANY_SSE4_2_FLAGS|CpuSSE4_1" },
  { "CPU_ANY_SSE4_2_FLAGS",
    "CpuSSE4_2" },
  { "CPU_ANY_AVX_FLAGS",
    "CPU_ANY_AVX2_FLAGS|CpuF16C|CpuFMA|CpuFMA4|CpuXOP|CpuAVX" },
  { "CPU_ANY_AVX2_FLAGS",
    "CpuAVX2" },
  { "CPU_ANY_AVX512F_FLAGS",
    "CpuVREX|CpuRegZMM|CpuRegMask|CpuAVX512CD|CpuAVX512ER|CpuAVX512PF|CpuAVX512DQ|CpuAVX512BW|CpuAVX512VL|CpuAVX512IFMA|CpuAVX512VBMI|CpuAVX512_4FMAPS|CpuAVX512_4VNNIW|CpuAVX512_VPOPCNTDQ|CpuAVX512F" },
  { "CPU_ANY_AVX512CD_FLAGS",
    "CpuAVX512CD" },
  { "CPU_ANY_AVX512ER_FLAGS",
    "CpuAVX512ER" },
  { "CPU_ANY_AVX512PF_FLAGS",
    "CpuAVX512PF" },
  { "CPU_ANY_AVX512DQ_FLAGS",
    "CpuAVX512DQ" },
  { "CPU_ANY_AVX512BW_FLAGS",
    "CpuAVX512BW" },
  { "CPU_ANY_AVX512VL_FLAGS",
    "CpuAVX512VL" },
  { "CPU_ANY_AVX512IFMA_FLAGS",
    "CpuAVX512IFMA" },
  { "CPU_ANY_AVX512VBMI_FLAGS",
    "CpuAVX512VBMI" },
  { "CPU_ANY_AVX512_4FMAPS_FLAGS",
    "CpuAVX512_4FMAPS" },
  { "CPU_ANY_AVX512_4VNNIW_FLAGS",
    "CpuAVX512_4VNNIW" },
  { "CPU_ANY_AVX512_VPOPCNTDQ_FLAGS",
    "CpuAVX512_VPOPCNTDQ" },
};

static initializer operand_type_init[] =
{
  { "OPERAND_TYPE_NONE",
    "0" },
  { "OPERAND_TYPE_REG8",
    "Reg8" },
  { "OPERAND_TYPE_REG16",
    "Reg16" },
  { "OPERAND_TYPE_REG32",
    "Reg32" },
  { "OPERAND_TYPE_REG64",
    "Reg64" },
  { "OPERAND_TYPE_IMM1",
    "Imm1" },
  { "OPERAND_TYPE_IMM8",
    "Imm8" },
  { "OPERAND_TYPE_IMM8S",
    "Imm8S" },
  { "OPERAND_TYPE_IMM16",
    "Imm16" },
  { "OPERAND_TYPE_IMM32",
    "Imm32" },
  { "OPERAND_TYPE_IMM32S",
    "Imm32S" },
  { "OPERAND_TYPE_IMM64",
    "Imm64" },
  { "OPERAND_TYPE_BASEINDEX",
    "BaseIndex" },
  { "OPERAND_TYPE_DISP8",
    "Disp8" },
  { "OPERAND_TYPE_DISP16",
    "Disp16" },
  { "OPERAND_TYPE_DISP32",
    "Disp32" },
  { "OPERAND_TYPE_DISP32S",
    "Disp32S" },
  { "OPERAND_TYPE_DISP64",
    "Disp64" },
  { "OPERAND_TYPE_INOUTPORTREG",
    "InOutPortReg" },
  { "OPERAND_TYPE_SHIFTCOUNT",
    "ShiftCount" },
  { "OPERAND_TYPE_CONTROL",
    "Control" },
  { "OPERAND_TYPE_TEST",
    "Test" },
  { "OPERAND_TYPE_DEBUG",
    "FloatReg" },
  { "OPERAND_TYPE_FLOATREG",
    "FloatReg" },
  { "OPERAND_TYPE_FLOATACC",
    "FloatAcc" },
  { "OPERAND_TYPE_SREG2",
    "SReg2" },
  { "OPERAND_TYPE_SREG3",
    "SReg3" },
  { "OPERAND_TYPE_ACC",
    "Acc" },
  { "OPERAND_TYPE_JUMPABSOLUTE",
    "JumpAbsolute" },
  { "OPERAND_TYPE_REGMMX",
    "RegMMX" },
  { "OPERAND_TYPE_REGXMM",
    "RegXMM" },
  { "OPERAND_TYPE_REGYMM",
    "RegYMM" },
  { "OPERAND_TYPE_REGZMM",
    "RegZMM" },
  { "OPERAND_TYPE_REGMASK",
    "RegMask" },
  { "OPERAND_TYPE_ESSEG",
    "EsSeg" },
  { "OPERAND_TYPE_ACC32",
    "Reg32|Acc|Dword" },
  { "OPERAND_TYPE_ACC64",
    "Reg64|Acc|Qword" },
  { "OPERAND_TYPE_INOUTPORTREG",
    "InOutPortReg" },
  { "OPERAND_TYPE_REG16_INOUTPORTREG",
    "Reg16|InOutPortReg" },
  { "OPERAND_TYPE_DISP16_32",
    "Disp16|Disp32" },
  { "OPERAND_TYPE_ANYDISP",
    "Disp8|Disp16|Disp32|Disp32S|Disp64" },
  { "OPERAND_TYPE_IMM16_32",
    "Imm16|Imm32" },
  { "OPERAND_TYPE_IMM16_32S",
    "Imm16|Imm32S" },
  { "OPERAND_TYPE_IMM16_32_32S",
    "Imm16|Imm32|Imm32S" },
  { "OPERAND_TYPE_IMM32_64",
    "Imm32|Imm64" },
  { "OPERAND_TYPE_IMM32_32S_DISP32",
    "Imm32|Imm32S|Disp32" },
  { "OPERAND_TYPE_IMM64_DISP64",
    "Imm64|Disp64" },
  { "OPERAND_TYPE_IMM32_32S_64_DISP32",
    "Imm32|Imm32S|Imm64|Disp32" },
  { "OPERAND_TYPE_IMM32_32S_64_DISP32_64",
    "Imm32|Imm32S|Imm64|Disp32|Disp64" },
  { "OPERAND_TYPE_VEC_IMM4",
    "Vec_Imm4" },
  { "OPERAND_TYPE_REGBND",
    "RegBND" },
  { "OPERAND_TYPE_VEC_DISP8",
    "Vec_Disp8" },
};

typedef struct bitfield
{
  int position;
  int value;
  const char *name;
} bitfield;

#define BITFIELD(n) { n, 0, #n }

static bitfield cpu_flags[] =
{
  BITFIELD (Cpu186),
  BITFIELD (Cpu286),
  BITFIELD (Cpu386),
  BITFIELD (Cpu486),
  BITFIELD (Cpu586),
  BITFIELD (Cpu686),
  BITFIELD (CpuClflush),
  BITFIELD (CpuNop),
  BITFIELD (CpuSYSCALL),
  BITFIELD (Cpu8087),
  BITFIELD (Cpu287),
  BITFIELD (Cpu387),
  BITFIELD (Cpu687),
  BITFIELD (CpuFISTTP),
  BITFIELD (CpuMMX),
  BITFIELD (CpuSSE),
  BITFIELD (CpuSSE2),
  BITFIELD (CpuSSE3),
  BITFIELD (CpuSSSE3),
  BITFIELD (CpuSSE4_1),
  BITFIELD (CpuSSE4_2),
  BITFIELD (CpuAVX),
  BITFIELD (CpuAVX2),
  BITFIELD (CpuAVX512F),
  BITFIELD (CpuAVX512CD),
  BITFIELD (CpuAVX512ER),
  BITFIELD (CpuAVX512PF),
  BITFIELD (CpuAVX512VL),
  BITFIELD (CpuAVX512DQ),
  BITFIELD (CpuAVX512BW),
  BITFIELD (CpuL1OM),
  BITFIELD (CpuK1OM),
  BITFIELD (CpuIAMCU),
  BITFIELD (CpuSSE4a),
  BITFIELD (Cpu3dnow),
  BITFIELD (Cpu3dnowA),
  BITFIELD (CpuPadLock),
  BITFIELD (CpuSVME),
  BITFIELD (CpuVMX),
  BITFIELD (CpuSMX),
  BITFIELD (CpuABM),
  BITFIELD (CpuXsave),
  BITFIELD (CpuXsaveopt),
  BITFIELD (CpuAES),
  BITFIELD (CpuPCLMUL),
  BITFIELD (CpuFMA),
  BITFIELD (CpuFMA4),
  BITFIELD (CpuXOP),
  BITFIELD (CpuLWP),
  BITFIELD (CpuBMI),
  BITFIELD (CpuTBM),
  BITFIELD (CpuLM),
  BITFIELD (CpuMovbe),
  BITFIELD (CpuCX16),
  BITFIELD (CpuEPT),
  BITFIELD (CpuRdtscp),
  BITFIELD (CpuFSGSBase),
  BITFIELD (CpuRdRnd),
  BITFIELD (CpuF16C),
  BITFIELD (CpuBMI2),
  BITFIELD (CpuLZCNT),
  BITFIELD (CpuHLE),
  BITFIELD (CpuRTM),
  BITFIELD (CpuINVPCID),
  BITFIELD (CpuVMFUNC),
  BITFIELD (CpuRDSEED),
  BITFIELD (CpuADX),
  BITFIELD (CpuPRFCHW),
  BITFIELD (CpuSMAP),
  BITFIELD (CpuSHA),
  BITFIELD (CpuVREX),
  BITFIELD (CpuClflushOpt),
  BITFIELD (CpuXSAVES),
  BITFIELD (CpuXSAVEC),
  BITFIELD (CpuPREFETCHWT1),
  BITFIELD (CpuSE1),
  BITFIELD (CpuCLWB),
  BITFIELD (Cpu64),
  BITFIELD (CpuNo64),
  BITFIELD (CpuMPX),
  BITFIELD (CpuAVX512IFMA),
  BITFIELD (CpuAVX512VBMI),
  BITFIELD (CpuAVX512_4FMAPS),
  BITFIELD (CpuAVX512_4VNNIW),
  BITFIELD (CpuAVX512_VPOPCNTDQ),
  BITFIELD (CpuMWAITX),
  BITFIELD (CpuCLZERO),
  BITFIELD (CpuOSPKE),
  BITFIELD (CpuRDPID),
  BITFIELD (CpuPTWRITE),
  BITFIELD (CpuCET),
  BITFIELD (CpuRegMMX),
  BITFIELD (CpuRegXMM),
  BITFIELD (CpuRegYMM),
  BITFIELD (CpuRegZMM),
  BITFIELD (CpuRegMask),
#ifdef CpuUnused
  BITFIELD (CpuUnused),
#endif
};

static bitfield opcode_modifiers[] =
{
  BITFIELD (D),
  BITFIELD (W),
  BITFIELD (Load),
  BITFIELD (Modrm),
  BITFIELD (ShortForm),
  BITFIELD (Jump),
  BITFIELD (JumpDword),
  BITFIELD (JumpByte),
  BITFIELD (JumpInterSegment),
  BITFIELD (FloatMF),
  BITFIELD (FloatR),
  BITFIELD (FloatD),
  BITFIELD (Size16),
  BITFIELD (Size32),
  BITFIELD (Size64),
  BITFIELD (CheckRegSize),
  BITFIELD (IgnoreSize),
  BITFIELD (DefaultSize),
  BITFIELD (No_bSuf),
  BITFIELD (No_wSuf),
  BITFIELD (No_lSuf),
  BITFIELD (No_sSuf),
  BITFIELD (No_qSuf),
  BITFIELD (No_ldSuf),
  BITFIELD (FWait),
  BITFIELD (IsString),
  BITFIELD (BNDPrefixOk),
  BITFIELD (IsLockable),
  BITFIELD (RegKludge),
  BITFIELD (FirstXmm0),
  BITFIELD (Implicit1stXmm0),
  BITFIELD (RepPrefixOk),
  BITFIELD (HLEPrefixOk),
  BITFIELD (ToDword),
  BITFIELD (ToQword),
  BITFIELD (AddrPrefixOp0),
  BITFIELD (IsPrefix),
  BITFIELD (ImmExt),
  BITFIELD (NoRex64),
  BITFIELD (Rex64),
  BITFIELD (Ugh),
  BITFIELD (Vex),
  BITFIELD (VexVVVV),
  BITFIELD (VexW),
  BITFIELD (VexOpcode),
  BITFIELD (VexSources),
  BITFIELD (VexImmExt),
  BITFIELD (VecSIB),
  BITFIELD (SSE2AVX),
  BITFIELD (NoAVX),
  BITFIELD (EVex),
  BITFIELD (Masking),
  BITFIELD (VecESize),
  BITFIELD (Broadcast),
  BITFIELD (StaticRounding),
  BITFIELD (SAE),
  BITFIELD (Disp8MemShift),
  BITFIELD (NoDefMask),
  BITFIELD (ImplicitQuadGroup),
  BITFIELD (OldGcc),
  BITFIELD (ATTMnemonic),
  BITFIELD (ATTSyntax),
  BITFIELD (IntelSyntax),
  BITFIELD (AMD64),
  BITFIELD (Intel64),
};

static bitfield operand_types[] =
{
  BITFIELD (Reg8),
  BITFIELD (Reg16),
  BITFIELD (Reg32),
  BITFIELD (Reg64),
  BITFIELD (FloatReg),
  BITFIELD (RegMMX),
  BITFIELD (RegXMM),
  BITFIELD (RegYMM),
  BITFIELD (RegZMM),
  BITFIELD (RegMask),
  BITFIELD (Imm1),
  BITFIELD (Imm8),
  BITFIELD (Imm8S),
  BITFIELD (Imm16),
  BITFIELD (Imm32),
  BITFIELD (Imm32S),
  BITFIELD (Imm64),
  BITFIELD (BaseIndex),
  BITFIELD (Disp8),
  BITFIELD (Disp16),
  BITFIELD (Disp32),
  BITFIELD (Disp32S),
  BITFIELD (Disp64),
  BITFIELD (InOutPortReg),
  BITFIELD (ShiftCount),
  BITFIELD (Control),
  BITFIELD (Debug),
  BITFIELD (Test),
  BITFIELD (SReg2),
  BITFIELD (SReg3),
  BITFIELD (Acc),
  BITFIELD (FloatAcc),
  BITFIELD (JumpAbsolute),
  BITFIELD (EsSeg),
  BITFIELD (RegMem),
  BITFIELD (Mem),
  BITFIELD (Byte),
  BITFIELD (Word),
  BITFIELD (Dword),
  BITFIELD (Fword),
  BITFIELD (Qword),
  BITFIELD (Tbyte),
  BITFIELD (Xmmword),
  BITFIELD (Ymmword),
  BITFIELD (Zmmword),
  BITFIELD (Unspecified),
  BITFIELD (Anysize),
  BITFIELD (Vec_Imm4),
  BITFIELD (RegBND),
  BITFIELD (Vec_Disp8),
#ifdef OTUnused
  BITFIELD (OTUnused),
#endif
};

static const char *filename;

static int
compare (const void *x, const void *y)
{
  const bitfield *xp = (const bitfield *) x;
  const bitfield *yp = (const bitfield *) y;
  return xp->position - yp->position;
}

static void
fail (const char *message, ...)
{
  va_list args;

  va_start (args, message);
  fprintf (stderr, _("%s: Error: "), program_name);
  vfprintf (stderr, message, args);
  va_end (args);
  xexit (1);
}

static void
process_copyright (FILE *fp)
{
  fprintf (fp, "/* This file is automatically generated by i386-gen.  Do not edit!  */\n\
/* Copyright (C) 2007-2017 Free Software Foundation, Inc.\n\
\n\
   This file is part of the GNU opcodes library.\n\
\n\
   This library is free software; you can redistribute it and/or modify\n\
   it under the terms of the GNU General Public License as published by\n\
   the Free Software Foundation; either version 3, or (at your option)\n\
   any later version.\n\
\n\
   It is distributed in the hope that it will be useful, but WITHOUT\n\
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY\n\
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public\n\
   License for more details.\n\
\n\
   You should have received a copy of the GNU General Public License\n\
   along with this program; if not, write to the Free Software\n\
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,\n\
   MA 02110-1301, USA.  */\n");
}

/* Remove leading white spaces.  */

static char *
remove_leading_whitespaces (char *str)
{
  while (ISSPACE (*str))
    str++;
  return str;
}

/* Remove trailing white spaces.  */

static void
remove_trailing_whitespaces (char *str)
{
  size_t last = strlen (str);

  if (last == 0)
    return;

  do
    {
      last--;
      if (ISSPACE (str [last]))
	str[last] = '\0';
      else
	break;
    }
  while (last != 0);
}

/* Find next field separated by SEP and terminate it. Return a
   pointer to the one after it.  */

static char *
next_field (char *str, char sep, char **next, char *last)
{
  char *p;

  p = remove_leading_whitespaces (str);
  for (str = p; *str != sep && *str != '\0'; str++);

  *str = '\0';
  remove_trailing_whitespaces (p);

  *next = str + 1;

  if (p >= last)
    abort ();

  return p;
}

static void set_bitfield (char *, bitfield *, int, unsigned int, int);

static int
set_bitfield_from_cpu_flag_init (char *f, bitfield *array,
				 int value, unsigned int size,
				 int lineno)
{
  char *str, *next, *last;
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE (cpu_flag_init); i++)
    if (strcmp (cpu_flag_init[i].name, f) == 0)
      {
	/* Turn on selective bits.  */
	char *init = xstrdup (cpu_flag_init[i].init);
	last = init + strlen (init);
	for (next = init; next && next < last; )
	  {
	    str = next_field (next, '|', &next, last);
	    if (str)
	      set_bitfield (str, array, 1, size, lineno);
	  }
	free (init);
	return 0;
      }

  return -1;
}

static void
set_bitfield (char *f, bitfield *array, int value,
	      unsigned int size, int lineno)
{
  unsigned int i;

  if (strcmp (f, "CpuFP") == 0)
    {
      set_bitfield("Cpu387", array, value, size, lineno);
      set_bitfield("Cpu287", array, value, size, lineno);
      f = "Cpu8087";
    }
  else if (strcmp (f, "Mmword") == 0)
    f= "Qword";
  else if (strcmp (f, "Oword") == 0)
    f= "Xmmword";

  for (i = 0; i < size; i++)
    if (strcasecmp (array[i].name, f) == 0)
      {
	array[i].value = value;
	return;
      }

  if (value)
    {
      const char *v = strchr (f, '=');

      if (v)
	{
	  size_t n = v - f;
	  char *end;

	  for (i = 0; i < size; i++)
	    if (strncasecmp (array[i].name, f, n) == 0)
	      {
		value = strtol (v + 1, &end, 0);
		if (*end == '\0')
		  {
		    array[i].value = value;
		    return;
		  }
		break;
	      }
	}
    }

  /* Handle CPU_XXX_FLAGS.  */
  if (!set_bitfield_from_cpu_flag_init (f, array, value, size, lineno))
    return;

  if (lineno != -1)
    fail (_("%s: %d: Unknown bitfield: %s\n"), filename, lineno, f);
  else
    fail (_("Unknown bitfield: %s\n"), f);
}

static void
output_cpu_flags (FILE *table, bitfield *flags, unsigned int size,
		  int macro, const char *comma, const char *indent)
{
  unsigned int i;

  fprintf (table, "%s{ { ", indent);

  for (i = 0; i < size - 1; i++)
    {
      if (((i + 1) % 20) != 0)
	fprintf (table, "%d, ", flags[i].value);
      else
	fprintf (table, "%d,", flags[i].value);
      if (((i + 1) % 20) == 0)
	{
	  /* We need \\ for macro.  */
	  if (macro)
	    fprintf (table, " \\\n    %s", indent);
	  else
	    fprintf (table, "\n    %s", indent);
	}
    }

  fprintf (table, "%d } }%s\n", flags[i].value, comma);
}

static void
process_i386_cpu_flag (FILE *table, char *flag, int macro,
		       const char *comma, const char *indent,
		       int lineno)
{
  char *str, *next, *last;
  unsigned int i;
  bitfield flags [ARRAY_SIZE (cpu_flags)];

  /* Copy the default cpu flags.  */
  memcpy (flags, cpu_flags, sizeof (cpu_flags));

  if (strcasecmp (flag, "unknown") == 0)
    {
      /* We turn on everything except for cpu64 in case of
	 CPU_UNKNOWN_FLAGS.  */
      for (i = 0; i < ARRAY_SIZE (flags); i++)
	if (flags[i].position != Cpu64)
	  flags[i].value = 1;
    }
  else if (flag[0] == '~')
    {
      last = flag + strlen (flag);

      if (flag[1] == '(')
	{
	  last -= 1;
	  next = flag + 2;
	  if (*last != ')')
	    fail (_("%s: %d: Missing `)' in bitfield: %s\n"), filename,
		  lineno, flag);
	  *last = '\0';
	}
      else
	next = flag + 1;

      /* First we turn on everything except for cpu64.  */
      for (i = 0; i < ARRAY_SIZE (flags); i++)
	if (flags[i].position != Cpu64)
	  flags[i].value = 1;

      /* Turn off selective bits.  */
      for (; next && next < last; )
	{
	  str = next_field (next, '|', &next, last);
	  if (str)
	    set_bitfield (str, flags, 0, ARRAY_SIZE (flags), lineno);
	}
    }
  else if (strcmp (flag, "0"))
    {
      /* Turn on selective bits.  */
      last = flag + strlen (flag);
      for (next = flag; next && next < last; )
	{
	  str = next_field (next, '|', &next, last);
	  if (str)
	    set_bitfield (str, flags, 1, ARRAY_SIZE (flags), lineno);
	}
    }

  output_cpu_flags (table, flags, ARRAY_SIZE (flags), macro,
		    comma, indent);
}

static void
output_opcode_modifier (FILE *table, bitfield *modifier, unsigned int size)
{
  unsigned int i;

  fprintf (table, "    { ");

  for (i = 0; i < size - 1; i++)
    {
      if (((i + 1) % 20) != 0)
        fprintf (table, "%d, ", modifier[i].value);
      else
        fprintf (table, "%d,", modifier[i].value);
      if (((i + 1) % 20) == 0)
	fprintf (table, "\n      ");
    }

  fprintf (table, "%d },\n", modifier[i].value);
}

static void
process_i386_opcode_modifier (FILE *table, char *mod, int lineno)
{
  char *str, *next, *last;
  bitfield modifiers [ARRAY_SIZE (opcode_modifiers)];

  /* Copy the default opcode modifier.  */
  memcpy (modifiers, opcode_modifiers, sizeof (modifiers));

  if (strcmp (mod, "0"))
    {
      last = mod + strlen (mod);
      for (next = mod; next && next < last; )
	{
	  str = next_field (next, '|', &next, last);
	  if (str)
	    set_bitfield (str, modifiers, 1, ARRAY_SIZE (modifiers),
			  lineno);
	}
    }
  output_opcode_modifier (table, modifiers, ARRAY_SIZE (modifiers));
}

static void
output_operand_type (FILE *table, bitfield *types, unsigned int size,
		     int macro, const char *indent)
{
  unsigned int i;

  fprintf (table, "{ { ");

  for (i = 0; i < size - 1; i++)
    {
      if (((i + 1) % 20) != 0)
	fprintf (table, "%d, ", types[i].value);
      else
	fprintf (table, "%d,", types[i].value);
      if (((i + 1) % 20) == 0)
	{
	  /* We need \\ for macro.  */
	  if (macro)
	    fprintf (table, " \\\n%s", indent);
	  else
	    fprintf (table, "\n%s", indent);
	}
    }

  fprintf (table, "%d } }", types[i].value);
}

static void
process_i386_operand_type (FILE *table, char *op, int macro,
			   const char *indent, int lineno)
{
  char *str, *next, *last;
  bitfield types [ARRAY_SIZE (operand_types)];

  /* Copy the default operand type.  */
  memcpy (types, operand_types, sizeof (types));

  if (strcmp (op, "0"))
    {
      last = op + strlen (op);
      for (next = op; next && next < last; )
	{
	  str = next_field (next, '|', &next, last);
	  if (str)
	    set_bitfield (str, types, 1, ARRAY_SIZE (types), lineno);
	}
    }
  output_operand_type (table, types, ARRAY_SIZE (types), macro,
		       indent);
}

static void
output_i386_opcode (FILE *table, const char *name, char *str,
		    char *last, int lineno)
{
  unsigned int i;
  char *operands, *base_opcode, *extension_opcode, *opcode_length;
  char *cpu_flags, *opcode_modifier, *operand_types [MAX_OPERANDS];

  /* Find number of operands.  */
  operands = next_field (str, ',', &str, last);

  /* Find base_opcode.  */
  base_opcode = next_field (str, ',', &str, last);

  /* Find extension_opcode.  */
  extension_opcode = next_field (str, ',', &str, last);

  /* Find opcode_length.  */
  opcode_length = next_field (str, ',', &str, last);

  /* Find cpu_flags.  */
  cpu_flags = next_field (str, ',', &str, last);

  /* Find opcode_modifier.  */
  opcode_modifier = next_field (str, ',', &str, last);

  /* Remove the first {.  */
  str = remove_leading_whitespaces (str);
  if (*str != '{')
    abort ();
  str = remove_leading_whitespaces (str + 1);

  i = strlen (str);

  /* There are at least "X}".  */
  if (i < 2)
    abort ();

  /* Remove trailing white spaces and }. */
  do
    {
      i--;
      if (ISSPACE (str[i]) || str[i] == '}')
	str[i] = '\0';
      else
	break;
    }
  while (i != 0);

  last = str + i;

  /* Find operand_types.  */
  for (i = 0; i < ARRAY_SIZE (operand_types); i++)
    {
      if (str >= last)
	{
	  operand_types [i] = NULL;
	  break;
	}

      operand_types [i] = next_field (str, ',', &str, last);
      if (*operand_types[i] == '0')
	{
	  if (i != 0)
	    operand_types[i] = NULL;
	  break;
	}
    }

  fprintf (table, "  { \"%s\", %s, %s, %s, %s,\n",
	   name, operands, base_opcode, extension_opcode,
	   opcode_length);

  process_i386_cpu_flag (table, cpu_flags, 0, ",", "    ", lineno);

  process_i386_opcode_modifier (table, opcode_modifier, lineno);

  fprintf (table, "    { ");

  for (i = 0; i < ARRAY_SIZE (operand_types); i++)
    {
      if (operand_types[i] == NULL || *operand_types[i] == '0')
	{
	  if (i == 0)
	    process_i386_operand_type (table, "0", 0, "\t  ", lineno);
	  break;
	}

      if (i != 0)
	fprintf (table, ",\n      ");

      process_i386_operand_type (table, operand_types[i], 0,
				 "\t  ", lineno);
    }
  fprintf (table, " } },\n");
}

struct opcode_hash_entry
{
  struct opcode_hash_entry *next;
  char *name;
  char *opcode;
  int lineno;
};

/* Calculate the hash value of an opcode hash entry P.  */

static hashval_t
opcode_hash_hash (const void *p)
{
  struct opcode_hash_entry *entry = (struct opcode_hash_entry *) p;
  return htab_hash_string (entry->name);
}

/* Compare a string Q against an opcode hash entry P.  */

static int
opcode_hash_eq (const void *p, const void *q)
{
  struct opcode_hash_entry *entry = (struct opcode_hash_entry *) p;
  const char *name = (const char *) q;
  return strcmp (name, entry->name) == 0;
}

static void
process_i386_opcodes (FILE *table)
{
  FILE *fp;
  char buf[2048];
  unsigned int i, j;
  char *str, *p, *last, *name;
  struct opcode_hash_entry **hash_slot, **entry, *next;
  htab_t opcode_hash_table;
  struct opcode_hash_entry **opcode_array;
  unsigned int opcode_array_size = 1024;
  int lineno = 0;

  filename = "i386-opc.tbl";
  fp = fopen (filename, "r");

  if (fp == NULL)
    fail (_("can't find i386-opc.tbl for reading, errno = %s\n"),
	  xstrerror (errno));

  i = 0;
  opcode_array = (struct opcode_hash_entry **)
    xmalloc (sizeof (*opcode_array) * opcode_array_size);

  opcode_hash_table = htab_create_alloc (16, opcode_hash_hash,
					 opcode_hash_eq, NULL,
					 xcalloc, free);

  fprintf (table, "\n/* i386 opcode table.  */\n\n");
  fprintf (table, "const insn_template i386_optab[] =\n{\n");

  /* Put everything on opcode array.  */
  while (!feof (fp))
    {
      if (fgets (buf, sizeof (buf), fp) == NULL)
	break;

      lineno++;

      p = remove_leading_whitespaces (buf);

      /* Skip comments.  */
      str = strstr (p, "//");
      if (str != NULL)
	str[0] = '\0';

      /* Remove trailing white spaces.  */
      remove_trailing_whitespaces (p);

      switch (p[0])
	{
	case '#':
	  /* Ignore comments.  */
	case '\0':
	  continue;
	  break;
	default:
	  break;
	}

      last = p + strlen (p);

      /* Find name.  */
      name = next_field (p, ',', &str, last);

      /* Get the slot in hash table.  */
      hash_slot = (struct opcode_hash_entry **)
	htab_find_slot_with_hash (opcode_hash_table, name,
				  htab_hash_string (name),
				  INSERT);

      if (*hash_slot == NULL)
	{
	  /* It is the new one.  Put it on opcode array.  */
	  if (i >= opcode_array_size)
	    {
	      /* Grow the opcode array when needed.  */
	      opcode_array_size += 1024;
	      opcode_array = (struct opcode_hash_entry **)
		xrealloc (opcode_array,
			  sizeof (*opcode_array) * opcode_array_size);
	    }

	  opcode_array[i] = (struct opcode_hash_entry *)
	    xmalloc (sizeof (struct opcode_hash_entry));
	  opcode_array[i]->next = NULL;
	  opcode_array[i]->name = xstrdup (name);
	  opcode_array[i]->opcode = xstrdup (str);
	  opcode_array[i]->lineno = lineno;
	  *hash_slot = opcode_array[i];
	  i++;
	}
      else
	{
	  /* Append it to the existing one.  */
	  entry = hash_slot;
	  while ((*entry) != NULL)
	    entry = &(*entry)->next;
	  *entry = (struct opcode_hash_entry *)
	    xmalloc (sizeof (struct opcode_hash_entry));
	  (*entry)->next = NULL;
	  (*entry)->name = (*hash_slot)->name;
	  (*entry)->opcode = xstrdup (str);
	  (*entry)->lineno = lineno;
	}
    }

  /* Process opcode array.  */
  for (j = 0; j < i; j++)
    {
      for (next = opcode_array[j]; next; next = next->next)
	{
	  name = next->name;
	  str = next->opcode;
	  lineno = next->lineno;
	  last = str + strlen (str);
	  output_i386_opcode (table, name, str, last, lineno);
	}
    }

  fclose (fp);

  fprintf (table, "  { NULL, 0, 0, 0, 0,\n");

  process_i386_cpu_flag (table, "0", 0, ",", "    ", -1);

  process_i386_opcode_modifier (table, "0", -1);

  fprintf (table, "    { ");
  process_i386_operand_type (table, "0", 0, "\t  ", -1);
  fprintf (table, " } }\n");

  fprintf (table, "};\n");
}

static void
process_i386_registers (FILE *table)
{
  FILE *fp;
  char buf[2048];
  char *str, *p, *last;
  char *reg_name, *reg_type, *reg_flags, *reg_num;
  char *dw2_32_num, *dw2_64_num;
  int lineno = 0;

  filename = "i386-reg.tbl";
  fp = fopen (filename, "r");
  if (fp == NULL)
    fail (_("can't find i386-reg.tbl for reading, errno = %s\n"),
	  xstrerror (errno));

  fprintf (table, "\n/* i386 register table.  */\n\n");
  fprintf (table, "const reg_entry i386_regtab[] =\n{\n");

  while (!feof (fp))
    {
      if (fgets (buf, sizeof (buf), fp) == NULL)
	break;

      lineno++;

      p = remove_leading_whitespaces (buf);

      /* Skip comments.  */
      str = strstr (p, "//");
      if (str != NULL)
	str[0] = '\0';

      /* Remove trailing white spaces.  */
      remove_trailing_whitespaces (p);

      switch (p[0])
	{
	case '#':
	  fprintf (table, "%s\n", p);
	case '\0':
	  continue;
	  break;
	default:
	  break;
	}

      last = p + strlen (p);

      /* Find reg_name.  */
      reg_name = next_field (p, ',', &str, last);

      /* Find reg_type.  */
      reg_type = next_field (str, ',', &str, last);

      /* Find reg_flags.  */
      reg_flags = next_field (str, ',', &str, last);

      /* Find reg_num.  */
      reg_num = next_field (str, ',', &str, last);

      fprintf (table, "  { \"%s\",\n    ", reg_name);

      process_i386_operand_type (table, reg_type, 0, "\t", lineno);

      /* Find 32-bit Dwarf2 register number.  */
      dw2_32_num = next_field (str, ',', &str, last);

      /* Find 64-bit Dwarf2 register number.  */
      dw2_64_num = next_field (str, ',', &str, last);

      fprintf (table, ",\n    %s, %s, { %s, %s } },\n",
	       reg_flags, reg_num, dw2_32_num, dw2_64_num);
    }

  fclose (fp);

  fprintf (table, "};\n");

  fprintf (table, "\nconst unsigned int i386_regtab_size = ARRAY_SIZE (i386_regtab);\n");
}

static void
process_i386_initializers (void)
{
  unsigned int i;
  FILE *fp = fopen ("i386-init.h", "w");
  char *init;

  if (fp == NULL)
    fail (_("can't create i386-init.h, errno = %s\n"),
	  xstrerror (errno));

  process_copyright (fp);

  for (i = 0; i < ARRAY_SIZE (cpu_flag_init); i++)
    {
      fprintf (fp, "\n#define %s \\\n", cpu_flag_init[i].name);
      init = xstrdup (cpu_flag_init[i].init);
      process_i386_cpu_flag (fp, init, 1, "", "  ", -1);
      free (init);
    }

  for (i = 0; i < ARRAY_SIZE (operand_type_init); i++)
    {
      fprintf (fp, "\n\n#define %s \\\n  ", operand_type_init[i].name);
      init = xstrdup (operand_type_init[i].init);
      process_i386_operand_type (fp, init, 1, "      ", -1);
      free (init);
    }
  fprintf (fp, "\n");

  fclose (fp);
}

/* Program options.  */
#define OPTION_SRCDIR	200

struct option long_options[] =
{
  {"srcdir",  required_argument, NULL, OPTION_SRCDIR},
  {"debug",   no_argument,       NULL, 'd'},
  {"version", no_argument,       NULL, 'V'},
  {"help",    no_argument,       NULL, 'h'},
  {0,         no_argument,       NULL, 0}
};

static void
print_version (void)
{
  printf ("%s: version 1.0\n", program_name);
  xexit (0);
}

static void
usage (FILE * stream, int status)
{
  fprintf (stream, "Usage: %s [-V | --version] [-d | --debug] [--srcdir=dirname] [--help]\n",
	   program_name);
  xexit (status);
}

int
main (int argc, char **argv)
{
  extern int chdir (char *);
  char *srcdir = NULL;
  int c;
  unsigned int i, cpumax;
  FILE *table;

  program_name = *argv;
  xmalloc_set_program_name (program_name);

  while ((c = getopt_long (argc, argv, "vVdh", long_options, 0)) != EOF)
    switch (c)
      {
      case OPTION_SRCDIR:
	srcdir = optarg;
	break;
      case 'V':
      case 'v':
	print_version ();
	break;
      case 'd':
	debug = 1;
	break;
      case 'h':
      case '?':
	usage (stderr, 0);
      default:
      case 0:
	break;
      }

  if (optind != argc)
    usage (stdout, 1);

  if (srcdir != NULL)
    if (chdir (srcdir) != 0)
      fail (_("unable to change directory to \"%s\", errno = %s\n"),
	    srcdir, xstrerror (errno));

  /* cpu_flags isn't sorted by position.  */
  cpumax = 0;
  for (i = 0; i < ARRAY_SIZE (cpu_flags); i++)
    if (cpu_flags[i].position > cpumax)
      cpumax = cpu_flags[i].position;

  /* Check the unused bitfield in i386_cpu_flags.  */
#ifdef CpuUnused
  if ((cpumax - 1) != CpuMax)
    fail (_("CpuMax != %d!\n"), cpumax);
#else
  if (cpumax != CpuMax)
    fail (_("CpuMax != %d!\n"), cpumax);

  c = CpuNumOfBits - CpuMax - 1;
  if (c)
    fail (_("%d unused bits in i386_cpu_flags.\n"), c);
#endif

  /* Check the unused bitfield in i386_operand_type.  */
#ifndef OTUnused
  c = OTNumOfBits - OTMax - 1;
  if (c)
    fail (_("%d unused bits in i386_operand_type.\n"), c);
#endif

  qsort (cpu_flags, ARRAY_SIZE (cpu_flags), sizeof (cpu_flags [0]),
	 compare);

  qsort (opcode_modifiers, ARRAY_SIZE (opcode_modifiers),
	 sizeof (opcode_modifiers [0]), compare);

  qsort (operand_types, ARRAY_SIZE (operand_types),
	 sizeof (operand_types [0]), compare);

  table = fopen ("i386-tbl.h", "w");
  if (table == NULL)
    fail (_("can't create i386-tbl.h, errno = %s\n"),
	  xstrerror (errno));

  process_copyright (table);

  process_i386_opcodes (table);
  process_i386_registers (table);
  process_i386_initializers ();

  fclose (table);

  exit (0);
}

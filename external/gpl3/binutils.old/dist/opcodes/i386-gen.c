/* Copyright (C) 2007-2022 Free Software Foundation, Inc.

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

/* Build-time checks are preferrable over runtime ones.  Use this construct
   in preference where possible.  */
#define static_assert(e) ((void)sizeof (struct { int _:1 - 2 * !(e); }))

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
    "~CpuIAMCU" },
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
    "CPU_I486_FLAGS|Cpu387|Cpu586" },
  { "CPU_I686_FLAGS",
    "CPU_I586_FLAGS|Cpu686|Cpu687|CpuCMOV|CpuFXSR" },
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
    "CPU_K8_FLAGS|CpuFISTTP|CPU_SSE4A_FLAGS|CpuLZCNT|CpuPOPCNT" },
  { "CPU_BDVER1_FLAGS",
    "CPU_GENERIC64_FLAGS|CpuFISTTP|CpuRdtscp|CpuCX16|CPU_XOP_FLAGS|CpuLZCNT|CpuPOPCNT|CpuLWP|CpuSVME|CpuAES|CpuPCLMUL|CpuPRFCHW" },
  { "CPU_BDVER2_FLAGS",
    "CPU_BDVER1_FLAGS|CpuFMA|CpuBMI|CpuTBM|CpuF16C" },
  { "CPU_BDVER3_FLAGS",
    "CPU_BDVER2_FLAGS|CpuXsaveopt|CpuFSGSBase" },
  { "CPU_BDVER4_FLAGS",
    "CPU_BDVER3_FLAGS|CpuAVX2|CpuMovbe|CpuBMI2|CpuRdRnd|CpuMWAITX" },
  { "CPU_ZNVER1_FLAGS",
    "CPU_GENERIC64_FLAGS|CpuFISTTP|CpuRdtscp|CpuCX16|CPU_AVX2_FLAGS|CpuSSE4A|CpuLZCNT|CpuPOPCNT|CpuSVME|CpuAES|CpuPCLMUL|CpuPRFCHW|CpuFMA|CpuBMI|CpuF16C|CpuXsaveopt|CpuFSGSBase|CpuMovbe|CpuBMI2|CpuRdRnd|CpuADX|CpuRdSeed|CpuSMAP|CpuSHA|CpuXSAVEC|CpuXSAVES|CpuClflushOpt|CpuCLZERO|CpuMWAITX" },
  { "CPU_ZNVER2_FLAGS",
    "CPU_ZNVER1_FLAGS|CpuCLWB|CpuRDPID|CpuRDPRU|CpuMCOMMIT|CpuWBNOINVD" },
  { "CPU_ZNVER3_FLAGS",
    "CPU_ZNVER2_FLAGS|CpuINVLPGB|CpuTLBSYNC|CpuVAES|CpuVPCLMULQDQ|CpuINVPCID|CpuSNP|CpuOSPKE" },
  { "CPU_BTVER1_FLAGS",
    "CPU_GENERIC64_FLAGS|CpuFISTTP|CpuCX16|CpuRdtscp|CPU_SSSE3_FLAGS|CpuSSE4A|CpuLZCNT|CpuPOPCNT|CpuPRFCHW|CpuCX16|CpuClflush|CpuFISTTP|CpuSVME" },
  { "CPU_BTVER2_FLAGS",
    "CPU_BTVER1_FLAGS|CPU_AVX_FLAGS|CpuBMI|CpuF16C|CpuAES|CpuPCLMUL|CpuMovbe|CpuXsaveopt|CpuPRFCHW" },
  { "CPU_8087_FLAGS",
    "Cpu8087" },
  { "CPU_287_FLAGS",
    "Cpu287" },
  { "CPU_387_FLAGS",
    "Cpu387" },
  { "CPU_687_FLAGS",
    "CPU_387_FLAGS|Cpu687" },
  { "CPU_CMOV_FLAGS",
    "CpuCMOV" },
  { "CPU_FXSR_FLAGS",
    "CpuFXSR" },
  { "CPU_CLFLUSH_FLAGS",
    "CpuClflush" },
  { "CPU_NOP_FLAGS",
    "CpuNop" },
  { "CPU_SYSCALL_FLAGS",
    "CpuSYSCALL" },
  { "CPU_MMX_FLAGS",
    "CpuMMX" },
  { "CPU_SSE_FLAGS",
    "CpuSSE" },
  { "CPU_SSE2_FLAGS",
    "CPU_SSE_FLAGS|CpuSSE2" },
  { "CPU_SSE3_FLAGS",
    "CPU_SSE2_FLAGS|CpuSSE3" },
  { "CPU_SSSE3_FLAGS",
    "CPU_SSE3_FLAGS|CpuSSSE3" },
  { "CPU_SSE4_1_FLAGS",
    "CPU_SSSE3_FLAGS|CpuSSE4_1" },
  { "CPU_SSE4_2_FLAGS",
    "CPU_SSE4_1_FLAGS|CpuSSE4_2|CpuPOPCNT" },
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
    "CPU_XSAVE_FLAGS|CpuLWP" },
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
  { "CPU_POPCNT_FLAGS",
    "CpuPOPCNT" },
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
    "CpuLZCNT|CpuPOPCNT" },
  { "CPU_AVX_FLAGS",
    "CPU_SSE4_2_FLAGS|CPU_XSAVE_FLAGS|CpuAVX" },
  { "CPU_AVX2_FLAGS",
    "CPU_AVX_FLAGS|CpuAVX2" },
  { "CPU_AVX_VNNI_FLAGS",
    "CPU_AVX2_FLAGS|CpuAVX_VNNI" },
  { "CPU_AVX512F_FLAGS",
    "CPU_AVX2_FLAGS|CpuAVX512F" },
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
    "CPU_AVX512F_FLAGS|CpuAVX512VL" },
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
  { "CPU_AVX512_VBMI2_FLAGS",
    "CPU_AVX512F_FLAGS|CpuAVX512_VBMI2" },
  { "CPU_AVX512_VNNI_FLAGS",
    "CPU_AVX512F_FLAGS|CpuAVX512_VNNI" },
  { "CPU_AVX512_BITALG_FLAGS",
    "CPU_AVX512F_FLAGS|CpuAVX512_BITALG" },
  { "CPU_AVX512_BF16_FLAGS",
    "CPU_AVX512F_FLAGS|CpuAVX512_BF16" },
  { "CPU_AVX512_FP16_FLAGS",
    "CPU_AVX512BW_FLAGS|CpuAVX512_FP16" },
  { "CPU_IAMCU_FLAGS",
    "Cpu186|Cpu286|Cpu386|Cpu486|Cpu586|CpuIAMCU" },
  { "CPU_ADX_FLAGS",
    "CpuADX" },
  { "CPU_RDSEED_FLAGS",
    "CpuRdSeed" },
  { "CPU_PRFCHW_FLAGS",
    "CpuPRFCHW" },
  { "CPU_SMAP_FLAGS",
    "CpuSMAP" },
  { "CPU_MPX_FLAGS",
    "CPU_XSAVE_FLAGS|CpuMPX" },
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
    "CPU_XSAVE_FLAGS|CpuOSPKE" },
  { "CPU_RDPID_FLAGS",
    "CpuRDPID" },
  { "CPU_PTWRITE_FLAGS",
    "CpuPTWRITE" },
  { "CPU_IBT_FLAGS",
    "CpuIBT" },
  { "CPU_SHSTK_FLAGS",
    "CpuSHSTK" },
  { "CPU_GFNI_FLAGS",
    "CpuGFNI" },
  { "CPU_VAES_FLAGS",
    "CpuVAES" },
  { "CPU_VPCLMULQDQ_FLAGS",
    "CpuVPCLMULQDQ" },
  { "CPU_WBNOINVD_FLAGS",
    "CpuWBNOINVD" },
  { "CPU_PCONFIG_FLAGS",
    "CpuPCONFIG" },
  { "CPU_WAITPKG_FLAGS",
    "CpuWAITPKG" },
  { "CPU_UINTR_FLAGS",
    "CpuUINTR" },
  { "CPU_CLDEMOTE_FLAGS",
    "CpuCLDEMOTE" },
  { "CPU_AMX_INT8_FLAGS",
    "CpuAMX_INT8" },
  { "CPU_AMX_BF16_FLAGS",
    "CpuAMX_BF16" },
  { "CPU_AMX_TILE_FLAGS",
    "CpuAMX_TILE" },
  { "CPU_MOVDIRI_FLAGS",
    "CpuMOVDIRI" },
  { "CPU_MOVDIR64B_FLAGS",
    "CpuMOVDIR64B" },
  { "CPU_ENQCMD_FLAGS",
    "CpuENQCMD" },
  { "CPU_SERIALIZE_FLAGS",
    "CpuSERIALIZE" },
  { "CPU_AVX512_VP2INTERSECT_FLAGS",
    "CpuAVX512_VP2INTERSECT" },
  { "CPU_TDX_FLAGS",
    "CpuTDX" },
  { "CPU_RDPRU_FLAGS",
    "CpuRDPRU" },
  { "CPU_MCOMMIT_FLAGS",
    "CpuMCOMMIT" },
  { "CPU_SEV_ES_FLAGS",
    "CpuSEV_ES" },
  { "CPU_TSXLDTRK_FLAGS",
    "CpuTSXLDTRK"},
  { "CPU_KL_FLAGS",
    "CpuKL" },
  { "CPU_WIDEKL_FLAGS",
    "CpuWideKL" },
  { "CPU_HRESET_FLAGS",
    "CpuHRESET"},
  { "CPU_INVLPGB_FLAGS",
    "CpuINVLPGB" },
  { "CPU_TLBSYNC_FLAGS",
    "CpuTLBSYNC" },
  { "CPU_SNP_FLAGS",
    "CpuSNP" },
  { "CPU_ANY_X87_FLAGS",
    "CPU_ANY_287_FLAGS|Cpu8087" },
  { "CPU_ANY_287_FLAGS",
    "CPU_ANY_387_FLAGS|Cpu287" },
  { "CPU_ANY_387_FLAGS",
    "CPU_ANY_687_FLAGS|Cpu387" },
  { "CPU_ANY_687_FLAGS",
    "Cpu687|CpuFISTTP" },
  { "CPU_ANY_CMOV_FLAGS",
    "CpuCMOV" },
  { "CPU_ANY_FXSR_FLAGS",
    "CpuFXSR" },
  { "CPU_ANY_MMX_FLAGS",
    "CPU_3DNOWA_FLAGS" },
  { "CPU_ANY_SSE_FLAGS",
    "CPU_ANY_SSE2_FLAGS|CpuSSE" },
  { "CPU_ANY_SSE2_FLAGS",
    "CPU_ANY_SSE3_FLAGS|CpuSSE2" },
  { "CPU_ANY_SSE3_FLAGS",
    "CPU_ANY_SSSE3_FLAGS|CpuSSE3|CpuSSE4a" },
  { "CPU_ANY_SSSE3_FLAGS",
    "CPU_ANY_SSE4_1_FLAGS|CpuSSSE3" },
  { "CPU_ANY_SSE4_1_FLAGS",
    "CPU_ANY_SSE4_2_FLAGS|CpuSSE4_1" },
  { "CPU_ANY_SSE4_2_FLAGS",
    "CpuSSE4_2" },
  { "CPU_ANY_SSE4A_FLAGS",
    "CpuSSE4a" },
  { "CPU_ANY_AVX_FLAGS",
    "CPU_ANY_AVX2_FLAGS|CpuF16C|CpuFMA|CpuFMA4|CpuXOP|CpuAVX" },
  { "CPU_ANY_AVX2_FLAGS",
    "CPU_ANY_AVX512F_FLAGS|CpuAVX2" },
  { "CPU_ANY_AVX512F_FLAGS",
    "CpuAVX512F|CpuAVX512CD|CpuAVX512ER|CpuAVX512PF|CpuAVX512DQ|CPU_ANY_AVX512BW_FLAGS|CpuAVX512VL|CpuAVX512IFMA|CpuAVX512VBMI|CpuAVX512_4FMAPS|CpuAVX512_4VNNIW|CpuAVX512_VPOPCNTDQ|CpuAVX512_VBMI2|CpuAVX512_VNNI|CpuAVX512_BITALG|CpuAVX512_BF16|CpuAVX512_VP2INTERSECT" },
  { "CPU_ANY_AVX512CD_FLAGS",
    "CpuAVX512CD" },
  { "CPU_ANY_AVX512ER_FLAGS",
    "CpuAVX512ER" },
  { "CPU_ANY_AVX512PF_FLAGS",
    "CpuAVX512PF" },
  { "CPU_ANY_AVX512DQ_FLAGS",
    "CpuAVX512DQ" },
  { "CPU_ANY_AVX512BW_FLAGS",
    "CpuAVX512BW|CPU_ANY_AVX512_FP16_FLAGS" },
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
  { "CPU_ANY_IBT_FLAGS",
    "CpuIBT" },
  { "CPU_ANY_SHSTK_FLAGS",
    "CpuSHSTK" },
  { "CPU_ANY_AVX512_VBMI2_FLAGS",
    "CpuAVX512_VBMI2" },
  { "CPU_ANY_AVX512_VNNI_FLAGS",
    "CpuAVX512_VNNI" },
  { "CPU_ANY_AVX512_BITALG_FLAGS",
    "CpuAVX512_BITALG" },
  { "CPU_ANY_AVX512_BF16_FLAGS",
    "CpuAVX512_BF16" },
  { "CPU_ANY_AMX_INT8_FLAGS",
    "CpuAMX_INT8" },
  { "CPU_ANY_AMX_BF16_FLAGS",
    "CpuAMX_BF16" },
  { "CPU_ANY_AMX_TILE_FLAGS",
    "CpuAMX_TILE|CpuAMX_INT8|CpuAMX_BF16" },
  { "CPU_ANY_AVX_VNNI_FLAGS",
    "CpuAVX_VNNI" },
  { "CPU_ANY_MOVDIRI_FLAGS",
    "CpuMOVDIRI" },
  { "CPU_ANY_UINTR_FLAGS",
    "CpuUINTR" },
  { "CPU_ANY_MOVDIR64B_FLAGS",
    "CpuMOVDIR64B" },
  { "CPU_ANY_ENQCMD_FLAGS",
    "CpuENQCMD" },
  { "CPU_ANY_SERIALIZE_FLAGS",
    "CpuSERIALIZE" },
  { "CPU_ANY_AVX512_VP2INTERSECT_FLAGS",
    "CpuAVX512_VP2INTERSECT" },
  { "CPU_ANY_TDX_FLAGS",
    "CpuTDX" },
  { "CPU_ANY_TSXLDTRK_FLAGS",
    "CpuTSXLDTRK" },
  { "CPU_ANY_KL_FLAGS",
    "CpuKL|CpuWideKL" },
  { "CPU_ANY_WIDEKL_FLAGS",
    "CpuWideKL" },
  { "CPU_ANY_HRESET_FLAGS",
    "CpuHRESET" },
  { "CPU_ANY_AVX512_FP16_FLAGS",
    "CpuAVX512_FP16" },
};

static initializer operand_type_init[] =
{
  { "OPERAND_TYPE_NONE",
    "0" },
  { "OPERAND_TYPE_REG8",
    "Class=Reg|Byte" },
  { "OPERAND_TYPE_REG16",
    "Class=Reg|Word" },
  { "OPERAND_TYPE_REG32",
    "Class=Reg|Dword" },
  { "OPERAND_TYPE_REG64",
    "Class=Reg|Qword" },
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
  { "OPERAND_TYPE_DISP64",
    "Disp64" },
  { "OPERAND_TYPE_INOUTPORTREG",
    "Instance=RegD|Word" },
  { "OPERAND_TYPE_SHIFTCOUNT",
    "Instance=RegC|Byte" },
  { "OPERAND_TYPE_CONTROL",
    "Class=RegCR" },
  { "OPERAND_TYPE_TEST",
    "Class=RegTR" },
  { "OPERAND_TYPE_DEBUG",
    "Class=RegDR" },
  { "OPERAND_TYPE_FLOATREG",
    "Class=Reg|Tbyte" },
  { "OPERAND_TYPE_FLOATACC",
    "Instance=Accum|Tbyte" },
  { "OPERAND_TYPE_SREG",
    "Class=SReg" },
  { "OPERAND_TYPE_REGMMX",
    "Class=RegMMX" },
  { "OPERAND_TYPE_REGXMM",
    "Class=RegSIMD|Xmmword" },
  { "OPERAND_TYPE_REGYMM",
    "Class=RegSIMD|Ymmword" },
  { "OPERAND_TYPE_REGZMM",
    "Class=RegSIMD|Zmmword" },
  { "OPERAND_TYPE_REGTMM",
    "Class=RegSIMD|Tmmword" },
  { "OPERAND_TYPE_REGMASK",
    "Class=RegMask" },
  { "OPERAND_TYPE_REGBND",
    "Class=RegBND" },
  { "OPERAND_TYPE_ACC8",
    "Instance=Accum|Byte" },
  { "OPERAND_TYPE_ACC16",
    "Instance=Accum|Word" },
  { "OPERAND_TYPE_ACC32",
    "Instance=Accum|Dword" },
  { "OPERAND_TYPE_ACC64",
    "Instance=Accum|Qword" },
  { "OPERAND_TYPE_DISP16_32",
    "Disp16|Disp32" },
  { "OPERAND_TYPE_ANYDISP",
    "Disp8|Disp16|Disp32|Disp64" },
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
  { "OPERAND_TYPE_ANYIMM",
    "Imm1|Imm8|Imm8S|Imm16|Imm32|Imm32S|Imm64" },
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
  BITFIELD (CpuCMOV),
  BITFIELD (CpuFXSR),
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
  BITFIELD (CpuIAMCU),
  BITFIELD (CpuSSE4a),
  BITFIELD (Cpu3dnow),
  BITFIELD (Cpu3dnowA),
  BITFIELD (CpuPadLock),
  BITFIELD (CpuSVME),
  BITFIELD (CpuVMX),
  BITFIELD (CpuSMX),
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
  BITFIELD (CpuPOPCNT),
  BITFIELD (CpuHLE),
  BITFIELD (CpuRTM),
  BITFIELD (CpuINVPCID),
  BITFIELD (CpuVMFUNC),
  BITFIELD (CpuRDSEED),
  BITFIELD (CpuADX),
  BITFIELD (CpuPRFCHW),
  BITFIELD (CpuSMAP),
  BITFIELD (CpuSHA),
  BITFIELD (CpuClflushOpt),
  BITFIELD (CpuXSAVES),
  BITFIELD (CpuXSAVEC),
  BITFIELD (CpuPREFETCHWT1),
  BITFIELD (CpuSE1),
  BITFIELD (CpuCLWB),
  BITFIELD (CpuMPX),
  BITFIELD (CpuAVX512IFMA),
  BITFIELD (CpuAVX512VBMI),
  BITFIELD (CpuAVX512_4FMAPS),
  BITFIELD (CpuAVX512_4VNNIW),
  BITFIELD (CpuAVX512_VPOPCNTDQ),
  BITFIELD (CpuAVX512_VBMI2),
  BITFIELD (CpuAVX512_VNNI),
  BITFIELD (CpuAVX512_BITALG),
  BITFIELD (CpuAVX512_BF16),
  BITFIELD (CpuAVX512_VP2INTERSECT),
  BITFIELD (CpuTDX),
  BITFIELD (CpuAVX_VNNI),
  BITFIELD (CpuAVX512_FP16),
  BITFIELD (CpuMWAITX),
  BITFIELD (CpuCLZERO),
  BITFIELD (CpuOSPKE),
  BITFIELD (CpuRDPID),
  BITFIELD (CpuPTWRITE),
  BITFIELD (CpuIBT),
  BITFIELD (CpuSHSTK),
  BITFIELD (CpuGFNI),
  BITFIELD (CpuVAES),
  BITFIELD (CpuVPCLMULQDQ),
  BITFIELD (CpuWBNOINVD),
  BITFIELD (CpuPCONFIG),
  BITFIELD (CpuWAITPKG),
  BITFIELD (CpuUINTR),
  BITFIELD (CpuCLDEMOTE),
  BITFIELD (CpuAMX_INT8),
  BITFIELD (CpuAMX_BF16),
  BITFIELD (CpuAMX_TILE),
  BITFIELD (CpuMOVDIRI),
  BITFIELD (CpuMOVDIR64B),
  BITFIELD (CpuENQCMD),
  BITFIELD (CpuSERIALIZE),
  BITFIELD (CpuRDPRU),
  BITFIELD (CpuMCOMMIT),
  BITFIELD (CpuSEV_ES),
  BITFIELD (CpuTSXLDTRK),
  BITFIELD (CpuKL),
  BITFIELD (CpuWideKL),
  BITFIELD (CpuHRESET),
  BITFIELD (CpuINVLPGB),
  BITFIELD (CpuTLBSYNC),
  BITFIELD (CpuSNP),
  BITFIELD (Cpu64),
  BITFIELD (CpuNo64),
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
  BITFIELD (Jump),
  BITFIELD (FloatMF),
  BITFIELD (FloatR),
  BITFIELD (Size),
  BITFIELD (CheckRegSize),
  BITFIELD (DistinctDest),
  BITFIELD (MnemonicSize),
  BITFIELD (Anysize),
  BITFIELD (No_bSuf),
  BITFIELD (No_wSuf),
  BITFIELD (No_lSuf),
  BITFIELD (No_sSuf),
  BITFIELD (No_qSuf),
  BITFIELD (No_ldSuf),
  BITFIELD (FWait),
  BITFIELD (IsString),
  BITFIELD (RegMem),
  BITFIELD (BNDPrefixOk),
  BITFIELD (RegKludge),
  BITFIELD (Implicit1stXmm0),
  BITFIELD (PrefixOk),
  BITFIELD (ToDword),
  BITFIELD (ToQword),
  BITFIELD (AddrPrefixOpReg),
  BITFIELD (IsPrefix),
  BITFIELD (ImmExt),
  BITFIELD (NoRex64),
  BITFIELD (Ugh),
  BITFIELD (PseudoVexPrefix),
  BITFIELD (Vex),
  BITFIELD (VexVVVV),
  BITFIELD (VexW),
  BITFIELD (OpcodeSpace),
  BITFIELD (OpcodePrefix),
  BITFIELD (VexSources),
  BITFIELD (SIB),
  BITFIELD (SSE2AVX),
  BITFIELD (EVex),
  BITFIELD (Masking),
  BITFIELD (Broadcast),
  BITFIELD (StaticRounding),
  BITFIELD (SAE),
  BITFIELD (Disp8MemShift),
  BITFIELD (NoDefMask),
  BITFIELD (ImplicitQuadGroup),
  BITFIELD (SwapSources),
  BITFIELD (Optimize),
  BITFIELD (ATTMnemonic),
  BITFIELD (ATTSyntax),
  BITFIELD (IntelSyntax),
  BITFIELD (ISA64),
};

#define CLASS(n) #n, n

static const struct {
  const char *name;
  enum operand_class value;
} operand_classes[] = {
  CLASS (Reg),
  CLASS (SReg),
  CLASS (RegCR),
  CLASS (RegDR),
  CLASS (RegTR),
  CLASS (RegMMX),
  CLASS (RegSIMD),
  CLASS (RegMask),
  CLASS (RegBND),
};

#undef CLASS

#define INSTANCE(n) #n, n

static const struct {
  const char *name;
  enum operand_instance value;
} operand_instances[] = {
    INSTANCE (Accum),
    INSTANCE (RegC),
    INSTANCE (RegD),
    INSTANCE (RegB),
};

#undef INSTANCE

static bitfield operand_types[] =
{
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
  BITFIELD (Disp64),
  BITFIELD (Byte),
  BITFIELD (Word),
  BITFIELD (Dword),
  BITFIELD (Fword),
  BITFIELD (Qword),
  BITFIELD (Tbyte),
  BITFIELD (Xmmword),
  BITFIELD (Ymmword),
  BITFIELD (Zmmword),
  BITFIELD (Tmmword),
  BITFIELD (Unspecified),
#ifdef OTUnused
  BITFIELD (OTUnused),
#endif
};

static const char *filename;
static i386_cpu_flags active_cpu_flags;
static int active_isstring;

struct template_arg {
  const struct template_arg *next;
  const char *val;
};

struct template_instance {
  const struct template_instance *next;
  const char *name;
  const struct template_arg *args;
};

struct template_param {
  const struct template_param *next;
  const char *name;
};

struct template {
  const struct template *next;
  const char *name;
  const struct template_instance *instances;
  const struct template_param *params;
};

static const struct template *templates;

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
  fprintf (stderr, _("%s: error: "), program_name);
  vfprintf (stderr, message, args);
  va_end (args);
  xexit (1);
}

static void
process_copyright (FILE *fp)
{
  fprintf (fp, "/* This file is automatically generated by i386-gen.  Do not edit!  */\n\
/* Copyright (C) 2007-2022 Free Software Foundation, Inc.\n\
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
set_bitfield_from_cpu_flag_init (char *f, bitfield *array, unsigned int size,
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

  /* Ignore empty fields; they may result from template expansions.  */
  if (*f == '\0')
    return;

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
  if (value == 1 && !set_bitfield_from_cpu_flag_init (f, array, size, lineno))
    return;

  if (lineno != -1)
    fail (_("%s: %d: unknown bitfield: %s\n"), filename, lineno, f);
  else
    fail (_("unknown bitfield: %s\n"), f);
}

static void
output_cpu_flags (FILE *table, bitfield *flags, unsigned int size,
		  int macro, const char *comma, const char *indent)
{
  unsigned int i;

  memset (&active_cpu_flags, 0, sizeof(active_cpu_flags));

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
      if (flags[i].value)
	active_cpu_flags.array[i / 32] |= 1U << (i % 32);
    }

  fprintf (table, "%d } }%s\n", flags[i].value, comma);
}

static void
process_i386_cpu_flag (FILE *table, char *flag, int macro,
		       const char *comma, const char *indent,
		       int lineno)
{
  char *str, *next = flag, *last;
  unsigned int i;
  int value = 1;
  bitfield flags [ARRAY_SIZE (cpu_flags)];

  /* Copy the default cpu flags.  */
  memcpy (flags, cpu_flags, sizeof (cpu_flags));

  if (flag[0] == '~')
    {
      last = flag + strlen (flag);

      if (flag[1] == '(')
	{
	  last -= 1;
	  next = flag + 2;
	  if (*last != ')')
	    fail (_("%s: %d: missing `)' in bitfield: %s\n"), filename,
		  lineno, flag);
	  *last = '\0';
	}
      else
	next = flag + 1;

      /* First we turn on everything except for cpu64, cpuno64, and - if
         present - the padding field.  */
      for (i = 0; i < ARRAY_SIZE (flags); i++)
	if (flags[i].position < Cpu64)
	  flags[i].value = 1;

      /* Turn off selective bits.  */
      value = 0;
    }

  if (strcmp (flag, "0"))
    {
      /* Turn on/off selective bits.  */
      last = flag + strlen (flag);
      for (; next && next < last; )
	{
	  str = next_field (next, '|', &next, last);
	  if (str)
	    set_bitfield (str, flags, value, ARRAY_SIZE (flags), lineno);
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

static int
adjust_broadcast_modifier (char **opnd)
{
  char *str, *next, *last, *op;
  int bcst_type = INT_MAX;

  /* Skip the immediate operand.  */
  op = opnd[0];
  if (strcasecmp(op, "Imm8") == 0)
    op = opnd[1];

  op = xstrdup (op);
  last = op + strlen (op);
  for (next = op; next && next < last; )
    {
      str = next_field (next, '|', &next, last);
      if (str)
	{
	  if (strcasecmp(str, "Byte") == 0)
	    {
	      /* The smalest broadcast type, no need to check
		 further.  */
	      bcst_type = BYTE_BROADCAST;
	      break;
	    }
	  else if (strcasecmp(str, "Word") == 0)
	    {
	      if (bcst_type > WORD_BROADCAST)
		bcst_type = WORD_BROADCAST;
	    }
	  else if (strcasecmp(str, "Dword") == 0)
	    {
	      if (bcst_type > DWORD_BROADCAST)
		bcst_type = DWORD_BROADCAST;
	    }
	  else if (strcasecmp(str, "Qword") == 0)
	    {
	      if (bcst_type > QWORD_BROADCAST)
		bcst_type = QWORD_BROADCAST;
	    }
	}
    }
  free (op);

  if (bcst_type == INT_MAX)
    fail (_("unknown broadcast operand: %s\n"), op);

  return bcst_type;
}

static void
process_i386_opcode_modifier (FILE *table, char *mod, unsigned int space,
			      unsigned int prefix, char **opnd, int lineno)
{
  char *str, *next, *last;
  bitfield modifiers [ARRAY_SIZE (opcode_modifiers)];

  active_isstring = 0;

  /* Copy the default opcode modifier.  */
  memcpy (modifiers, opcode_modifiers, sizeof (modifiers));

  if (strcmp (mod, "0"))
    {
      unsigned int have_w = 0, bwlq_suf = 0xf;

      last = mod + strlen (mod);
      for (next = mod; next && next < last; )
	{
	  str = next_field (next, '|', &next, last);
	  if (str)
	    {
	      int val = 1;
	      if (strcasecmp(str, "Broadcast") == 0)
		val = adjust_broadcast_modifier (opnd);

	      set_bitfield (str, modifiers, val, ARRAY_SIZE (modifiers),
			    lineno);
	      if (strcasecmp(str, "IsString") == 0)
		active_isstring = 1;

	      if (strcasecmp(str, "W") == 0)
		have_w = 1;

	      if (strcasecmp(str, "No_bSuf") == 0)
		bwlq_suf &= ~1;
	      if (strcasecmp(str, "No_wSuf") == 0)
		bwlq_suf &= ~2;
	      if (strcasecmp(str, "No_lSuf") == 0)
		bwlq_suf &= ~4;
	      if (strcasecmp(str, "No_qSuf") == 0)
		bwlq_suf &= ~8;
	    }
	}

      if (space)
	{
	  if (!modifiers[OpcodeSpace].value)
	    modifiers[OpcodeSpace].value = space;
	  else if (modifiers[OpcodeSpace].value != space)
	    fail (_("%s:%d: Conflicting opcode space specifications\n"),
		  filename, lineno);
	  else
	    fprintf (stderr,
		     _("%s:%d: Warning: redundant opcode space specification\n"),
		     filename, lineno);
	}

      if (prefix)
	{
	  if (!modifiers[OpcodePrefix].value)
	    modifiers[OpcodePrefix].value = prefix;
	  else if (modifiers[OpcodePrefix].value != prefix)
	    fail (_("%s:%d: Conflicting prefix specifications\n"),
		  filename, lineno);
	  else
	    fprintf (stderr,
		     _("%s:%d: Warning: redundant prefix specification\n"),
		     filename, lineno);
	}

      if (have_w && !bwlq_suf)
	fail ("%s: %d: stray W modifier\n", filename, lineno);
      if (have_w && !(bwlq_suf & 1))
	fprintf (stderr, "%s: %d: W modifier without Byte operand(s)\n",
		 filename, lineno);
      if (have_w && !(bwlq_suf & ~1))
	fprintf (stderr,
		 "%s: %d: W modifier without Word/Dword/Qword operand(s)\n",
		 filename, lineno);
    }
  output_opcode_modifier (table, modifiers, ARRAY_SIZE (modifiers));
}

enum stage {
  stage_macros,
  stage_opcodes,
  stage_registers,
};

static void
output_operand_type (FILE *table, enum operand_class class,
		     enum operand_instance instance,
		     const bitfield *types, unsigned int size,
		     enum stage stage, const char *indent)
{
  unsigned int i;

  fprintf (table, "{ { %d, %d, ", class, instance);

  for (i = 0; i < size - 1; i++)
    {
      if (((i + 3) % 20) != 0)
	fprintf (table, "%d, ", types[i].value);
      else
	fprintf (table, "%d,", types[i].value);
      if (((i + 3) % 20) == 0)
	{
	  /* We need \\ for macro.  */
	  if (stage == stage_macros)
	    fprintf (table, " \\\n%s", indent);
	  else
	    fprintf (table, "\n%s", indent);
	}
    }

  fprintf (table, "%d } }", types[i].value);
}

static void
process_i386_operand_type (FILE *table, char *op, enum stage stage,
			   const char *indent, int lineno)
{
  char *str, *next, *last;
  enum operand_class class = ClassNone;
  enum operand_instance instance = InstanceNone;
  bitfield types [ARRAY_SIZE (operand_types)];

  /* Copy the default operand type.  */
  memcpy (types, operand_types, sizeof (types));

  if (strcmp (op, "0"))
    {
      int baseindex = 0;

      last = op + strlen (op);
      for (next = op; next && next < last; )
	{
	  str = next_field (next, '|', &next, last);
	  if (str)
	    {
	      unsigned int i;

	      if (!strncmp(str, "Class=", 6))
		{
		  for (i = 0; i < ARRAY_SIZE(operand_classes); ++i)
		    if (!strcmp(str + 6, operand_classes[i].name))
		      {
			class = operand_classes[i].value;
			str = NULL;
			break;
		      }
		}

	      if (str && !strncmp(str, "Instance=", 9))
		{
		  for (i = 0; i < ARRAY_SIZE(operand_instances); ++i)
		    if (!strcmp(str + 9, operand_instances[i].name))
		      {
			instance = operand_instances[i].value;
			str = NULL;
			break;
		      }
		}
	    }
	  if (str)
	    {
	      set_bitfield (str, types, 1, ARRAY_SIZE (types), lineno);
	      if (strcasecmp(str, "BaseIndex") == 0)
		baseindex = 1;
	    }
	}

      if (stage == stage_opcodes && baseindex && !active_isstring)
	{
	  set_bitfield("Disp8", types, 1, ARRAY_SIZE (types), lineno);
	  if (!active_cpu_flags.bitfield.cpu64
	      && !active_cpu_flags.bitfield.cpumpx)
	    set_bitfield("Disp16", types, 1, ARRAY_SIZE (types), lineno);
	  set_bitfield("Disp32", types, 1, ARRAY_SIZE (types), lineno);
	}
    }
  output_operand_type (table, class, instance, types, ARRAY_SIZE (types),
		       stage, indent);
}

static void
output_i386_opcode (FILE *table, const char *name, char *str,
		    char *last, int lineno)
{
  unsigned int i, length, prefix = 0, space = 0;
  char *base_opcode, *extension_opcode, *end;
  char *cpu_flags, *opcode_modifier, *operand_types [MAX_OPERANDS];
  unsigned long long opcode;

  /* Find base_opcode.  */
  base_opcode = next_field (str, ',', &str, last);

  /* Find extension_opcode.  */
  extension_opcode = next_field (str, ',', &str, last);

  /* Find cpu_flags.  */
  cpu_flags = next_field (str, ',', &str, last);

  /* Find opcode_modifier.  */
  opcode_modifier = next_field (str, ',', &str, last);

  /* Remove the first {.  */
  str = remove_leading_whitespaces (str);
  if (*str != '{')
    abort ();
  str = remove_leading_whitespaces (str + 1);
  remove_trailing_whitespaces (str);

  /* Remove } and trailing white space. */
  i = strlen (str);
  if (!i || str[i - 1] != '}')
    abort ();
  str[--i] = '\0';
  remove_trailing_whitespaces (str);

  if (!*str)
    operand_types [i = 0] = NULL;
  else
    {
      last = str + strlen (str);

      /* Find operand_types.  */
      for (i = 0; i < ARRAY_SIZE (operand_types); i++)
	{
	  if (str >= last)
	    {
	      operand_types [i] = NULL;
	      break;
	    }

	  operand_types [i] = next_field (str, ',', &str, last);
	}
    }

  opcode = strtoull (base_opcode, &end, 0);

  /* Determine opcode length.  */
  for (length = 1; length < 8; ++length)
    if (!(opcode >> (8 * length)))
       break;

  /* Transform prefixes encoded in the opcode into opcode modifier
     representation.  */
  if (length > 1)
    {
      switch (opcode >> (8 * length - 8))
	{
	case 0x66: prefix = PREFIX_0X66; break;
	case 0xF3: prefix = PREFIX_0XF3; break;
	case 0xF2: prefix = PREFIX_0XF2; break;
	}

      if (prefix)
	opcode &= (1ULL << (8 * --length)) - 1;
    }

  /* Transform opcode space encoded in the opcode into opcode modifier
     representation.  */
  if (length > 1 && (opcode >> (8 * length - 8)) == 0xf)
    {
      switch ((opcode >> (8 * length - 16)) & 0xff)
	{
	default:   space = SPACE_0F;   break;
	case 0x38: space = SPACE_0F38; break;
	case 0x3A: space = SPACE_0F3A; break;
	}

      if (space != SPACE_0F && --length == 1)
	fail (_("%s:%d: %s: unrecognized opcode encoding space\n"),
	      filename, lineno, name);
      opcode &= (1ULL << (8 * --length)) - 1;
    }

  if (length > 2)
    fail (_("%s:%d: %s: residual opcode (0x%0*llx) too large\n"),
	  filename, lineno, name, 2 * length, opcode);

  fprintf (table, "  { \"%s\", 0x%0*llx%s, %s, %lu,\n",
	   name, 2 * (int)length, opcode, end, extension_opcode, i);

  process_i386_opcode_modifier (table, opcode_modifier, space, prefix,
				operand_types, lineno);

  process_i386_cpu_flag (table, cpu_flags, 0, ",", "    ", lineno);

  fprintf (table, "    { ");

  for (i = 0; i < ARRAY_SIZE (operand_types); i++)
    {
      if (!operand_types[i])
	{
	  if (i == 0)
	    process_i386_operand_type (table, "0", stage_opcodes, "\t  ",
				       lineno);
	  break;
	}

      if (i != 0)
	fprintf (table, ",\n      ");

      process_i386_operand_type (table, operand_types[i], stage_opcodes,
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
parse_template (char *buf, int lineno)
{
  char sep, *end, *name;
  struct template *tmpl = xmalloc (sizeof (*tmpl));
  struct template_instance *last_inst = NULL;

  buf = remove_leading_whitespaces (buf + 1);
  end = strchr (buf, ':');
  if (end == NULL)
    fail ("%s: %d: missing ':'\n", filename, lineno);
  *end++ = '\0';
  remove_trailing_whitespaces (buf);

  if (*buf == '\0')
    fail ("%s: %d: missing template identifier\n", filename, lineno);
  tmpl->name = xstrdup (buf);

  tmpl->params = NULL;
  do {
      struct template_param *param;

      buf = remove_leading_whitespaces (end);
      end = strpbrk (buf, ":,");
      if (end == NULL)
        fail ("%s: %d: missing ':' or ','\n", filename, lineno);

      sep = *end;
      *end++ = '\0';
      remove_trailing_whitespaces (buf);

      param = xmalloc (sizeof (*param));
      param->name = xstrdup (buf);
      param->next = tmpl->params;
      tmpl->params = param;
  } while (sep == ':');

  tmpl->instances = NULL;
  do {
      struct template_instance *inst;
      char *cur, *next;
      const struct template_param *param;

      buf = remove_leading_whitespaces (end);
      end = strpbrk (buf, ",>");
      if (end == NULL)
        fail ("%s: %d: missing ',' or '>'\n", filename, lineno);

      sep = *end;
      *end++ = '\0';

      inst = xmalloc (sizeof (*inst));
      inst->next = NULL;
      inst->args = NULL;

      cur = next_field (buf, ':', &next, end);
      inst->name = *cur != '$' ? xstrdup (cur) : "";

      for (param = tmpl->params; param; param = param->next)
	{
	  struct template_arg *arg = xmalloc (sizeof (*arg));

	  cur = next_field (next, ':', &next, end);
	  if (next > end)
	    fail ("%s: %d: missing argument for '%s'\n", filename, lineno, param->name);
	  arg->val = xstrdup (cur);
	  arg->next = inst->args;
	  inst->args = arg;
	}

      if (tmpl->instances)
	last_inst->next = inst;
      else
	tmpl->instances = inst;
      last_inst = inst;
  } while (sep == ',');

  buf = remove_leading_whitespaces (end);
  if (*buf)
    fprintf(stderr, "%s: %d: excess characters '%s'\n",
	    filename, lineno, buf);

  tmpl->next = templates;
  templates = tmpl;
}

static unsigned int
expand_templates (char *name, const char *str, htab_t opcode_hash_table,
		  struct opcode_hash_entry ***opcode_array_p, int lineno)
{
  static unsigned int idx, opcode_array_size;
  struct opcode_hash_entry **opcode_array = *opcode_array_p;
  struct opcode_hash_entry **hash_slot, **entry;
  char *ptr1 = strchr(name, '<'), *ptr2;

  if (ptr1 == NULL)
    {
      /* Get the slot in hash table.  */
      hash_slot = (struct opcode_hash_entry **)
	htab_find_slot_with_hash (opcode_hash_table, name,
				  htab_hash_string (name),
				  INSERT);

      if (*hash_slot == NULL)
	{
	  /* It is the new one.  Put it on opcode array.  */
	  if (idx >= opcode_array_size)
	    {
	      /* Grow the opcode array when needed.  */
	      opcode_array_size += 1024;
	      opcode_array = (struct opcode_hash_entry **)
		xrealloc (opcode_array,
			  sizeof (*opcode_array) * opcode_array_size);
		*opcode_array_p = opcode_array;
	    }

	  opcode_array[idx] = (struct opcode_hash_entry *)
	    xmalloc (sizeof (struct opcode_hash_entry));
	  opcode_array[idx]->next = NULL;
	  opcode_array[idx]->name = xstrdup (name);
	  opcode_array[idx]->opcode = xstrdup (str);
	  opcode_array[idx]->lineno = lineno;
	  *hash_slot = opcode_array[idx];
	  idx++;
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
  else if ((ptr2 = strchr(ptr1 + 1, '>')) == NULL)
    fail ("%s: %d: missing '>'\n", filename, lineno);
  else
    {
      const struct template *tmpl;
      const struct template_instance *inst;

      *ptr1 = '\0';
      ptr1 = remove_leading_whitespaces (ptr1 + 1);
      remove_trailing_whitespaces (ptr1);

      *ptr2++ = '\0';

      for ( tmpl = templates; tmpl; tmpl = tmpl->next )
	if (!strcmp(ptr1, tmpl->name))
	  break;
      if (!tmpl)
	fail ("reference to unknown template '%s'\n", ptr1);

      for (inst = tmpl->instances; inst; inst = inst->next)
	{
	  char *name2 = xmalloc(strlen(name) + strlen(inst->name) + strlen(ptr2) + 1);
	  char *str2 = xmalloc(2 * strlen(str));
	  const char *src;

	  strcpy (name2, name);
	  strcat (name2, inst->name);
	  strcat (name2, ptr2);

	  for (ptr1 = str2, src = str; *src; )
	    {
	      const char *ident = tmpl->name, *end;
	      const struct template_param *param;
	      const struct template_arg *arg;

	      if ((*ptr1 = *src++) != '<')
		{
		  ++ptr1;
		  continue;
		}
	      while (ISSPACE(*src))
		++src;
	      while (*ident && *src == *ident)
		++src, ++ident;
	      while (ISSPACE(*src))
		++src;
	      if (*src != ':' || *ident != '\0')
		{
		  memcpy (++ptr1, tmpl->name, ident - tmpl->name);
		  ptr1 += ident - tmpl->name;
		  continue;
		}
	      while (ISSPACE(*++src))
		;

	      end = src;
	      while (*end != '\0' && !ISSPACE(*end) && *end != '>')
		++end;

	      for (param = tmpl->params, arg = inst->args; param;
		   param = param->next, arg = arg->next)
		{
		  if (end - src == strlen (param->name)
		      && !memcmp (src, param->name, end - src))
		    {
		      src = end;
		      break;
		    }
		}

	      if (param == NULL)
		fail ("template '%s' has no parameter '%.*s'\n",
		      tmpl->name, (int)(end - src), src);

	      while (ISSPACE(*src))
		++src;
	      if (*src != '>')
		fail ("%s: %d: missing '>'\n", filename, lineno);

	      memcpy(ptr1, arg->val, strlen(arg->val));
	      ptr1 += strlen(arg->val);
	      ++src;
	    }

	  *ptr1 = '\0';

	  expand_templates (name2, str2, opcode_hash_table, opcode_array_p,
			    lineno);

	  free (str2);
	  free (name2);
	}
    }

  return idx;
}

static void
process_i386_opcodes (FILE *table)
{
  FILE *fp;
  char buf[2048];
  unsigned int i, j;
  char *str, *p, *last, *name;
  htab_t opcode_hash_table;
  struct opcode_hash_entry **opcode_array = NULL;
  int lineno = 0, marker = 0;

  filename = "i386-opc.tbl";
  fp = stdin;

  i = 0;
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
	  if (!strcmp("### MARKER ###", buf))
	    marker = 1;
	  else
	    {
	      /* Since we ignore all included files (we only care about their
		 #define-s here), we don't need to monitor filenames.  The final
		 line number directive is going to refer to the main source file
		 again.  */
	      char *end;
	      unsigned long ln;

	      p = remove_leading_whitespaces (p + 1);
	      if (!strncmp(p, "line", 4))
		p += 4;
	      ln = strtoul (p, &end, 10);
	      if (ln > 1 && ln < INT_MAX
		  && *remove_leading_whitespaces (end) == '"')
		lineno = ln - 1;
	    }
	  /* Ignore comments.  */
	case '\0':
	  continue;
	  break;
	case '<':
	  parse_template (p, lineno);
	  continue;
	default:
	  if (!marker)
	    continue;
	  break;
	}

      last = p + strlen (p);

      /* Find name.  */
      name = next_field (p, ',', &str, last);

      i = expand_templates (name, str, opcode_hash_table, &opcode_array,
			    lineno);
    }

  /* Process opcode array.  */
  for (j = 0; j < i; j++)
    {
      struct opcode_hash_entry *next;

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

  fprintf (table, "  { NULL, 0, 0, 0,\n");

  process_i386_opcode_modifier (table, "0", 0, 0, NULL, -1);

  process_i386_cpu_flag (table, "0", 0, ",", "    ", -1);

  fprintf (table, "    { ");
  process_i386_operand_type (table, "0", stage_opcodes, "\t  ", -1);
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

      process_i386_operand_type (table, reg_type, stage_registers, "\t",
				 lineno);

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
      process_i386_operand_type (fp, init, stage_macros, "      ", -1);
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
  static_assert (ARRAY_SIZE (cpu_flags) == CpuMax + 2);

  if ((cpumax - 1) != CpuMax)
    fail (_("CpuMax != %d!\n"), cpumax);
#else
  static_assert (ARRAY_SIZE (cpu_flags) == CpuMax + 1);

  if (cpumax != CpuMax)
    fail (_("CpuMax != %d!\n"), cpumax);

  c = CpuNumOfBits - CpuMax - 1;
  if (c)
    fail (_("%d unused bits in i386_cpu_flags.\n"), c);
#endif

  static_assert (ARRAY_SIZE (opcode_modifiers) == Opcode_Modifier_Num);

  /* Check the unused bitfield in i386_operand_type.  */
#ifdef OTUnused
  static_assert (ARRAY_SIZE (operand_types) + CLASS_WIDTH + INSTANCE_WIDTH
		 == OTNum + 1);
#else
  static_assert (ARRAY_SIZE (operand_types) + CLASS_WIDTH + INSTANCE_WIDTH
		 == OTNum);

  c = OTNumOfBits - OTNum;
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

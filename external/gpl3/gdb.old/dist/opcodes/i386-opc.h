/* Declarations for Intel 80386 opcode table
   Copyright (C) 2007-2017 Free Software Foundation, Inc.

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
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include "opcode/i386.h"
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

/* Position of cpu flags bitfiled.  */

enum
{
  /* i186 or better required */
  Cpu186 = 0,
  /* i286 or better required */
  Cpu286,
  /* i386 or better required */
  Cpu386,
  /* i486 or better required */
  Cpu486,
  /* i585 or better required */
  Cpu586,
  /* i686 or better required */
  Cpu686,
  /* CLFLUSH Instruction support required */
  CpuClflush,
  /* NOP Instruction support required */
  CpuNop,
  /* SYSCALL Instructions support required */
  CpuSYSCALL,
  /* Floating point support required */
  Cpu8087,
  /* i287 support required */
  Cpu287,
  /* i387 support required */
  Cpu387,
  /* i686 and floating point support required */
  Cpu687,
  /* SSE3 and floating point support required */
  CpuFISTTP,
  /* MMX support required */
  CpuMMX,
  /* SSE support required */
  CpuSSE,
  /* SSE2 support required */
  CpuSSE2,
  /* 3dnow! support required */
  Cpu3dnow,
  /* 3dnow! Extensions support required */
  Cpu3dnowA,
  /* SSE3 support required */
  CpuSSE3,
  /* VIA PadLock required */
  CpuPadLock,
  /* AMD Secure Virtual Machine Ext-s required */
  CpuSVME,
  /* VMX Instructions required */
  CpuVMX,
  /* SMX Instructions required */
  CpuSMX,
  /* SSSE3 support required */
  CpuSSSE3,
  /* SSE4a support required */
  CpuSSE4a,
  /* ABM New Instructions required */
  CpuABM,
  /* SSE4.1 support required */
  CpuSSE4_1,
  /* SSE4.2 support required */
  CpuSSE4_2,
  /* AVX support required */
  CpuAVX,
  /* AVX2 support required */
  CpuAVX2,
  /* Intel AVX-512 Foundation Instructions support required */
  CpuAVX512F,
  /* Intel AVX-512 Conflict Detection Instructions support required */
  CpuAVX512CD,
  /* Intel AVX-512 Exponential and Reciprocal Instructions support
     required */
  CpuAVX512ER,
  /* Intel AVX-512 Prefetch Instructions support required */
  CpuAVX512PF,
  /* Intel AVX-512 VL Instructions support required.  */
  CpuAVX512VL,
  /* Intel AVX-512 DQ Instructions support required.  */
  CpuAVX512DQ,
  /* Intel AVX-512 BW Instructions support required.  */
  CpuAVX512BW,
  /* Intel L1OM support required */
  CpuL1OM,
  /* Intel K1OM support required */
  CpuK1OM,
  /* Intel IAMCU support required */
  CpuIAMCU,
  /* Xsave/xrstor New Instructions support required */
  CpuXsave,
  /* Xsaveopt New Instructions support required */
  CpuXsaveopt,
  /* AES support required */
  CpuAES,
  /* PCLMUL support required */
  CpuPCLMUL,
  /* FMA support required */
  CpuFMA,
  /* FMA4 support required */
  CpuFMA4,
  /* XOP support required */
  CpuXOP,
  /* LWP support required */
  CpuLWP,
  /* BMI support required */
  CpuBMI,
  /* TBM support required */
  CpuTBM,
  /* MOVBE Instruction support required */
  CpuMovbe,
  /* CMPXCHG16B instruction support required.  */
  CpuCX16,
  /* EPT Instructions required */
  CpuEPT,
  /* RDTSCP Instruction support required */
  CpuRdtscp,
  /* FSGSBASE Instructions required */
  CpuFSGSBase,
  /* RDRND Instructions required */
  CpuRdRnd,
  /* F16C Instructions required */
  CpuF16C,
  /* Intel BMI2 support required */
  CpuBMI2,
  /* LZCNT support required */
  CpuLZCNT,
  /* HLE support required */
  CpuHLE,
  /* RTM support required */
  CpuRTM,
  /* INVPCID Instructions required */
  CpuINVPCID,
  /* VMFUNC Instruction required */
  CpuVMFUNC,
  /* Intel MPX Instructions required  */
  CpuMPX,
  /* 64bit support available, used by -march= in assembler.  */
  CpuLM,
  /* RDRSEED instruction required.  */
  CpuRDSEED,
  /* Multi-presisionn add-carry instructions are required.  */
  CpuADX,
  /* Supports prefetchw and prefetch instructions.  */
  CpuPRFCHW,
  /* SMAP instructions required.  */
  CpuSMAP,
  /* SHA instructions required.  */
  CpuSHA,
  /* VREX support required  */
  CpuVREX,
  /* CLFLUSHOPT instruction required */
  CpuClflushOpt,
  /* XSAVES/XRSTORS instruction required */
  CpuXSAVES,
  /* XSAVEC instruction required */
  CpuXSAVEC,
  /* PREFETCHWT1 instruction required */
  CpuPREFETCHWT1,
  /* SE1 instruction required */
  CpuSE1,
  /* CLWB instruction required */
  CpuCLWB,
  /* Intel AVX-512 IFMA Instructions support required.  */
  CpuAVX512IFMA,
  /* Intel AVX-512 VBMI Instructions support required.  */
  CpuAVX512VBMI,
  /* Intel AVX-512 4FMAPS Instructions support required.  */
  CpuAVX512_4FMAPS,
  /* Intel AVX-512 4VNNIW Instructions support required.  */
  CpuAVX512_4VNNIW,
  /* Intel AVX-512 VPOPCNTDQ Instructions support required.  */
  CpuAVX512_VPOPCNTDQ,
  /* mwaitx instruction required */
  CpuMWAITX,
  /* Clzero instruction required */
  CpuCLZERO,
  /* OSPKE instruction required */
  CpuOSPKE,
  /* RDPID instruction required */
  CpuRDPID,
  /* PTWRITE instruction required */
  CpuPTWRITE,
  /* CET instruction support required */
  CpuCET,
  /* MMX register support required */
  CpuRegMMX,
  /* XMM register support required */
  CpuRegXMM,
  /* YMM register support required */
  CpuRegYMM,
  /* ZMM register support required */
  CpuRegZMM,
  /* Mask register support required */
  CpuRegMask,
  /* 64bit support required  */
  Cpu64,
  /* Not supported in the 64bit mode  */
  CpuNo64,
  /* The last bitfield in i386_cpu_flags.  */
  CpuMax = CpuNo64
};

#define CpuNumOfUints \
  (CpuMax / sizeof (unsigned int) / CHAR_BIT + 1)
#define CpuNumOfBits \
  (CpuNumOfUints * sizeof (unsigned int) * CHAR_BIT)

/* If you get a compiler error for zero width of the unused field,
   comment it out.  */
#if 0
#define CpuUnused	(CpuMax + 1)
#endif

/* We can check if an instruction is available with array instead
   of bitfield. */
typedef union i386_cpu_flags
{
  struct
    {
      unsigned int cpui186:1;
      unsigned int cpui286:1;
      unsigned int cpui386:1;
      unsigned int cpui486:1;
      unsigned int cpui586:1;
      unsigned int cpui686:1;
      unsigned int cpuclflush:1;
      unsigned int cpunop:1;
      unsigned int cpusyscall:1;
      unsigned int cpu8087:1;
      unsigned int cpu287:1;
      unsigned int cpu387:1;
      unsigned int cpu687:1;
      unsigned int cpufisttp:1;
      unsigned int cpummx:1;
      unsigned int cpusse:1;
      unsigned int cpusse2:1;
      unsigned int cpua3dnow:1;
      unsigned int cpua3dnowa:1;
      unsigned int cpusse3:1;
      unsigned int cpupadlock:1;
      unsigned int cpusvme:1;
      unsigned int cpuvmx:1;
      unsigned int cpusmx:1;
      unsigned int cpussse3:1;
      unsigned int cpusse4a:1;
      unsigned int cpuabm:1;
      unsigned int cpusse4_1:1;
      unsigned int cpusse4_2:1;
      unsigned int cpuavx:1;
      unsigned int cpuavx2:1;
      unsigned int cpuavx512f:1;
      unsigned int cpuavx512cd:1;
      unsigned int cpuavx512er:1;
      unsigned int cpuavx512pf:1;
      unsigned int cpuavx512vl:1;
      unsigned int cpuavx512dq:1;
      unsigned int cpuavx512bw:1;
      unsigned int cpul1om:1;
      unsigned int cpuk1om:1;
      unsigned int cpuiamcu:1;
      unsigned int cpuxsave:1;
      unsigned int cpuxsaveopt:1;
      unsigned int cpuaes:1;
      unsigned int cpupclmul:1;
      unsigned int cpufma:1;
      unsigned int cpufma4:1;
      unsigned int cpuxop:1;
      unsigned int cpulwp:1;
      unsigned int cpubmi:1;
      unsigned int cputbm:1;
      unsigned int cpumovbe:1;
      unsigned int cpucx16:1;
      unsigned int cpuept:1;
      unsigned int cpurdtscp:1;
      unsigned int cpufsgsbase:1;
      unsigned int cpurdrnd:1;
      unsigned int cpuf16c:1;
      unsigned int cpubmi2:1;
      unsigned int cpulzcnt:1;
      unsigned int cpuhle:1;
      unsigned int cpurtm:1;
      unsigned int cpuinvpcid:1;
      unsigned int cpuvmfunc:1;
      unsigned int cpumpx:1;
      unsigned int cpulm:1;
      unsigned int cpurdseed:1;
      unsigned int cpuadx:1;
      unsigned int cpuprfchw:1;
      unsigned int cpusmap:1;
      unsigned int cpusha:1;
      unsigned int cpuvrex:1;
      unsigned int cpuclflushopt:1;
      unsigned int cpuxsaves:1;
      unsigned int cpuxsavec:1;
      unsigned int cpuprefetchwt1:1;
      unsigned int cpuse1:1;
      unsigned int cpuclwb:1;
      unsigned int cpuavx512ifma:1;
      unsigned int cpuavx512vbmi:1;
      unsigned int cpuavx512_4fmaps:1;
      unsigned int cpuavx512_4vnniw:1;
      unsigned int cpuavx512_vpopcntdq:1;
      unsigned int cpumwaitx:1;
      unsigned int cpuclzero:1;
      unsigned int cpuospke:1;
      unsigned int cpurdpid:1;
      unsigned int cpuptwrite:1;
      unsigned int cpucet:1;
      unsigned int cpuregmmx:1;
      unsigned int cpuregxmm:1;
      unsigned int cpuregymm:1;
      unsigned int cpuregzmm:1;
      unsigned int cpuregmask:1;
      unsigned int cpu64:1;
      unsigned int cpuno64:1;
#ifdef CpuUnused
      unsigned int unused:(CpuNumOfBits - CpuUnused);
#endif
    } bitfield;
  unsigned int array[CpuNumOfUints];
} i386_cpu_flags;

/* Position of opcode_modifier bits.  */

enum
{
  /* has direction bit. */
  D = 0,
  /* set if operands can be words or dwords encoded the canonical way */
  W,
  /* load form instruction. Must be placed before store form.  */
  Load,
  /* insn has a modrm byte. */
  Modrm,
  /* register is in low 3 bits of opcode */
  ShortForm,
  /* special case for jump insns.  */
  Jump,
  /* call and jump */
  JumpDword,
  /* loop and jecxz */
  JumpByte,
  /* special case for intersegment leaps/calls */
  JumpInterSegment,
  /* FP insn memory format bit, sized by 0x4 */
  FloatMF,
  /* src/dest swap for floats. */
  FloatR,
  /* has float insn direction bit. */
  FloatD,
  /* needs size prefix if in 32-bit mode */
  Size16,
  /* needs size prefix if in 16-bit mode */
  Size32,
  /* needs size prefix if in 64-bit mode */
  Size64,
  /* check register size.  */
  CheckRegSize,
  /* instruction ignores operand size prefix and in Intel mode ignores
     mnemonic size suffix check.  */
  IgnoreSize,
  /* default insn size depends on mode */
  DefaultSize,
  /* b suffix on instruction illegal */
  No_bSuf,
  /* w suffix on instruction illegal */
  No_wSuf,
  /* l suffix on instruction illegal */
  No_lSuf,
  /* s suffix on instruction illegal */
  No_sSuf,
  /* q suffix on instruction illegal */
  No_qSuf,
  /* long double suffix on instruction illegal */
  No_ldSuf,
  /* instruction needs FWAIT */
  FWait,
  /* quick test for string instructions */
  IsString,
  /* quick test if branch instruction is MPX supported */
  BNDPrefixOk,
  /* quick test for lockable instructions */
  IsLockable,
  /* fake an extra reg operand for clr, imul and special register
     processing for some instructions.  */
  RegKludge,
  /* The first operand must be xmm0 */
  FirstXmm0,
  /* An implicit xmm0 as the first operand */
  Implicit1stXmm0,
  /* The HLE prefix is OK:
     1. With a LOCK prefix.
     2. With or without a LOCK prefix.
     3. With a RELEASE (0xf3) prefix.
   */
#define HLEPrefixNone		0
#define HLEPrefixLock		1
#define HLEPrefixAny		2
#define HLEPrefixRelease	3
  HLEPrefixOk,
  /* An instruction on which a "rep" prefix is acceptable.  */
  RepPrefixOk,
  /* Convert to DWORD */
  ToDword,
  /* Convert to QWORD */
  ToQword,
  /* Address prefix changes operand 0 */
  AddrPrefixOp0,
  /* opcode is a prefix */
  IsPrefix,
  /* instruction has extension in 8 bit imm */
  ImmExt,
  /* instruction don't need Rex64 prefix.  */
  NoRex64,
  /* instruction require Rex64 prefix.  */
  Rex64,
  /* deprecated fp insn, gets a warning */
  Ugh,
  /* insn has VEX prefix:
	1: 128bit VEX prefix.
	2: 256bit VEX prefix.
	3: Scalar VEX prefix.
   */
#define VEX128		1
#define VEX256		2
#define VEXScalar	3
  Vex,
  /* How to encode VEX.vvvv:
     0: VEX.vvvv must be 1111b.
     1: VEX.NDS.  Register-only source is encoded in VEX.vvvv where
	the content of source registers will be preserved.
	VEX.DDS.  The second register operand is encoded in VEX.vvvv
	where the content of first source register will be overwritten
	by the result.
	VEX.NDD2.  The second destination register operand is encoded in
	VEX.vvvv for instructions with 2 destination register operands.
	For assembler, there are no difference between VEX.NDS, VEX.DDS
	and VEX.NDD2.
     2. VEX.NDD.  Register destination is encoded in VEX.vvvv for
     instructions with 1 destination register operand.
     3. VEX.LWP.  Register destination is encoded in VEX.vvvv and one
	of the operands can access a memory location.
   */
#define VEXXDS	1
#define VEXNDD	2
#define VEXLWP	3
  VexVVVV,
  /* How the VEX.W bit is used:
     0: Set by the REX.W bit.
     1: VEX.W0.  Should always be 0.
     2: VEX.W1.  Should always be 1.
   */
#define VEXW0	1
#define VEXW1	2
  VexW,
  /* VEX opcode prefix:
     0: VEX 0x0F opcode prefix.
     1: VEX 0x0F38 opcode prefix.
     2: VEX 0x0F3A opcode prefix
     3: XOP 0x08 opcode prefix.
     4: XOP 0x09 opcode prefix
     5: XOP 0x0A opcode prefix.
   */
#define VEX0F		0
#define VEX0F38		1
#define VEX0F3A		2
#define XOP08		3
#define XOP09		4
#define XOP0A		5
  VexOpcode,
  /* number of VEX source operands:
     0: <= 2 source operands.
     1: 2 XOP source operands.
     2: 3 source operands.
   */
#define XOP2SOURCES	1
#define VEX3SOURCES	2
  VexSources,
  /* instruction has VEX 8 bit imm */
  VexImmExt,
  /* Instruction with vector SIB byte:
	1: 128bit vector register.
	2: 256bit vector register.
	3: 512bit vector register.
   */
#define VecSIB128	1
#define VecSIB256	2
#define VecSIB512	3
  VecSIB,
  /* SSE to AVX support required */
  SSE2AVX,
  /* No AVX equivalent */
  NoAVX,

  /* insn has EVEX prefix:
	1: 512bit EVEX prefix.
	2: 128bit EVEX prefix.
	3: 256bit EVEX prefix.
	4: Length-ignored (LIG) EVEX prefix.
   */
#define EVEX512                1
#define EVEX128                2
#define EVEX256                3
#define EVEXLIG                4
  EVex,

  /* AVX512 masking support:
	1: Zeroing-masking.
	2: Merging-masking.
	3: Both zeroing and merging masking.
   */
#define ZEROING_MASKING 1
#define MERGING_MASKING 2
#define BOTH_MASKING    3
  Masking,

  /* Input element size of vector insn:
	0: 32bit.
	1: 64bit.
   */
  VecESize,

  /* Broadcast factor.
	0: No broadcast.
	1: 1to16 broadcast.
	2: 1to8 broadcast.
   */
#define NO_BROADCAST	0
#define BROADCAST_1TO16	1
#define BROADCAST_1TO8	2
#define BROADCAST_1TO4	3
#define BROADCAST_1TO2	4
  Broadcast,

  /* Static rounding control is supported.  */
  StaticRounding,

  /* Supress All Exceptions is supported.  */
  SAE,

  /* Copressed Disp8*N attribute.  */
  Disp8MemShift,

  /* Default mask isn't allowed.  */
  NoDefMask,

  /* The second operand must be a vector register, {x,y,z}mmN, where N is a multiple of 4.
     It implicitly denotes the register group of {x,y,z}mmN - {x,y,z}mm(N + 3).
   */
  ImplicitQuadGroup,

  /* Compatible with old (<= 2.8.1) versions of gcc  */
  OldGcc,
  /* AT&T mnemonic.  */
  ATTMnemonic,
  /* AT&T syntax.  */
  ATTSyntax,
  /* Intel syntax.  */
  IntelSyntax,
  /* AMD64.  */
  AMD64,
  /* Intel64.  */
  Intel64,
  /* The last bitfield in i386_opcode_modifier.  */
  Opcode_Modifier_Max
};

typedef struct i386_opcode_modifier
{
  unsigned int d:1;
  unsigned int w:1;
  unsigned int load:1;
  unsigned int modrm:1;
  unsigned int shortform:1;
  unsigned int jump:1;
  unsigned int jumpdword:1;
  unsigned int jumpbyte:1;
  unsigned int jumpintersegment:1;
  unsigned int floatmf:1;
  unsigned int floatr:1;
  unsigned int floatd:1;
  unsigned int size16:1;
  unsigned int size32:1;
  unsigned int size64:1;
  unsigned int checkregsize:1;
  unsigned int ignoresize:1;
  unsigned int defaultsize:1;
  unsigned int no_bsuf:1;
  unsigned int no_wsuf:1;
  unsigned int no_lsuf:1;
  unsigned int no_ssuf:1;
  unsigned int no_qsuf:1;
  unsigned int no_ldsuf:1;
  unsigned int fwait:1;
  unsigned int isstring:1;
  unsigned int bndprefixok:1;
  unsigned int islockable:1;
  unsigned int regkludge:1;
  unsigned int firstxmm0:1;
  unsigned int implicit1stxmm0:1;
  unsigned int hleprefixok:2;
  unsigned int repprefixok:1;
  unsigned int todword:1;
  unsigned int toqword:1;
  unsigned int addrprefixop0:1;
  unsigned int isprefix:1;
  unsigned int immext:1;
  unsigned int norex64:1;
  unsigned int rex64:1;
  unsigned int ugh:1;
  unsigned int vex:2;
  unsigned int vexvvvv:2;
  unsigned int vexw:2;
  unsigned int vexopcode:3;
  unsigned int vexsources:2;
  unsigned int veximmext:1;
  unsigned int vecsib:2;
  unsigned int sse2avx:1;
  unsigned int noavx:1;
  unsigned int evex:3;
  unsigned int masking:2;
  unsigned int vecesize:1;
  unsigned int broadcast:3;
  unsigned int staticrounding:1;
  unsigned int sae:1;
  unsigned int disp8memshift:3;
  unsigned int nodefmask:1;
  unsigned int implicitquadgroup:1;
  unsigned int oldgcc:1;
  unsigned int attmnemonic:1;
  unsigned int attsyntax:1;
  unsigned int intelsyntax:1;
  unsigned int amd64:1;
  unsigned int intel64:1;
} i386_opcode_modifier;

/* Position of operand_type bits.  */

enum
{
  /* 8bit register */
  Reg8 = 0,
  /* 16bit register */
  Reg16,
  /* 32bit register */
  Reg32,
  /* 64bit register */
  Reg64,
  /* Floating pointer stack register */
  FloatReg,
  /* MMX register */
  RegMMX,
  /* SSE register */
  RegXMM,
  /* AVX registers */
  RegYMM,
  /* AVX512 registers */
  RegZMM,
  /* Vector Mask registers */
  RegMask,
  /* Control register */
  Control,
  /* Debug register */
  Debug,
  /* Test register */
  Test,
  /* 2 bit segment register */
  SReg2,
  /* 3 bit segment register */
  SReg3,
  /* 1 bit immediate */
  Imm1,
  /* 8 bit immediate */
  Imm8,
  /* 8 bit immediate sign extended */
  Imm8S,
  /* 16 bit immediate */
  Imm16,
  /* 32 bit immediate */
  Imm32,
  /* 32 bit immediate sign extended */
  Imm32S,
  /* 64 bit immediate */
  Imm64,
  /* 8bit/16bit/32bit displacements are used in different ways,
     depending on the instruction.  For jumps, they specify the
     size of the PC relative displacement, for instructions with
     memory operand, they specify the size of the offset relative
     to the base register, and for instructions with memory offset
     such as `mov 1234,%al' they specify the size of the offset
     relative to the segment base.  */
  /* 8 bit displacement */
  Disp8,
  /* 16 bit displacement */
  Disp16,
  /* 32 bit displacement */
  Disp32,
  /* 32 bit signed displacement */
  Disp32S,
  /* 64 bit displacement */
  Disp64,
  /* Accumulator %al/%ax/%eax/%rax */
  Acc,
  /* Floating pointer top stack register %st(0) */
  FloatAcc,
  /* Register which can be used for base or index in memory operand.  */
  BaseIndex,
  /* Register to hold in/out port addr = dx */
  InOutPortReg,
  /* Register to hold shift count = cl */
  ShiftCount,
  /* Absolute address for jump.  */
  JumpAbsolute,
  /* String insn operand with fixed es segment */
  EsSeg,
  /* RegMem is for instructions with a modrm byte where the register
     destination operand should be encoded in the mod and regmem fields.
     Normally, it will be encoded in the reg field. We add a RegMem
     flag to the destination register operand to indicate that it should
     be encoded in the regmem field.  */
  RegMem,
  /* Memory.  */
  Mem,
  /* BYTE memory. */
  Byte,
  /* WORD memory. 2 byte */
  Word,
  /* DWORD memory. 4 byte */
  Dword,
  /* FWORD memory. 6 byte */
  Fword,
  /* QWORD memory. 8 byte */
  Qword,
  /* TBYTE memory. 10 byte */
  Tbyte,
  /* XMMWORD memory. */
  Xmmword,
  /* YMMWORD memory. */
  Ymmword,
  /* ZMMWORD memory.  */
  Zmmword,
  /* Unspecified memory size.  */
  Unspecified,
  /* Any memory size.  */
  Anysize,

  /* Vector 4 bit immediate.  */
  Vec_Imm4,

  /* Bound register.  */
  RegBND,

  /* Vector 8bit displacement */
  Vec_Disp8,

  /* The last bitfield in i386_operand_type.  */
  OTMax
};

#define OTNumOfUints \
  (OTMax / sizeof (unsigned int) / CHAR_BIT + 1)
#define OTNumOfBits \
  (OTNumOfUints * sizeof (unsigned int) * CHAR_BIT)

/* If you get a compiler error for zero width of the unused field,
   comment it out.  */
#define OTUnused		(OTMax + 1)

typedef union i386_operand_type
{
  struct
    {
      unsigned int reg8:1;
      unsigned int reg16:1;
      unsigned int reg32:1;
      unsigned int reg64:1;
      unsigned int floatreg:1;
      unsigned int regmmx:1;
      unsigned int regxmm:1;
      unsigned int regymm:1;
      unsigned int regzmm:1;
      unsigned int regmask:1;
      unsigned int control:1;
      unsigned int debug:1;
      unsigned int test:1;
      unsigned int sreg2:1;
      unsigned int sreg3:1;
      unsigned int imm1:1;
      unsigned int imm8:1;
      unsigned int imm8s:1;
      unsigned int imm16:1;
      unsigned int imm32:1;
      unsigned int imm32s:1;
      unsigned int imm64:1;
      unsigned int disp8:1;
      unsigned int disp16:1;
      unsigned int disp32:1;
      unsigned int disp32s:1;
      unsigned int disp64:1;
      unsigned int acc:1;
      unsigned int floatacc:1;
      unsigned int baseindex:1;
      unsigned int inoutportreg:1;
      unsigned int shiftcount:1;
      unsigned int jumpabsolute:1;
      unsigned int esseg:1;
      unsigned int regmem:1;
      unsigned int mem:1;
      unsigned int byte:1;
      unsigned int word:1;
      unsigned int dword:1;
      unsigned int fword:1;
      unsigned int qword:1;
      unsigned int tbyte:1;
      unsigned int xmmword:1;
      unsigned int ymmword:1;
      unsigned int zmmword:1;
      unsigned int unspecified:1;
      unsigned int anysize:1;
      unsigned int vec_imm4:1;
      unsigned int regbnd:1;
      unsigned int vec_disp8:1;
#ifdef OTUnused
      unsigned int unused:(OTNumOfBits - OTUnused);
#endif
    } bitfield;
  unsigned int array[OTNumOfUints];
} i386_operand_type;

typedef struct insn_template
{
  /* instruction name sans width suffix ("mov" for movl insns) */
  char *name;

  /* how many operands */
  unsigned int operands;

  /* base_opcode is the fundamental opcode byte without optional
     prefix(es).  */
  unsigned int base_opcode;
#define Opcode_D	0x2 /* Direction bit:
			       set if Reg --> Regmem;
			       unset if Regmem --> Reg. */
#define Opcode_FloatR	0x8 /* Bit to swap src/dest for float insns. */
#define Opcode_FloatD 0x400 /* Direction bit for float insns. */

  /* extension_opcode is the 3 bit extension for group <n> insns.
     This field is also used to store the 8-bit opcode suffix for the
     AMD 3DNow! instructions.
     If this template has no extension opcode (the usual case) use None
     Instructions */
  unsigned int extension_opcode;
#define None 0xffff		/* If no extension_opcode is possible.  */

  /* Opcode length.  */
  unsigned char opcode_length;

  /* cpu feature flags */
  i386_cpu_flags cpu_flags;

  /* the bits in opcode_modifier are used to generate the final opcode from
     the base_opcode.  These bits also are used to detect alternate forms of
     the same instruction */
  i386_opcode_modifier opcode_modifier;

  /* operand_types[i] describes the type of operand i.  This is made
     by OR'ing together all of the possible type masks.  (e.g.
     'operand_types[i] = Reg|Imm' specifies that operand i can be
     either a register or an immediate operand.  */
  i386_operand_type operand_types[MAX_OPERANDS];
}
insn_template;

extern const insn_template i386_optab[];

/* these are for register name --> number & type hash lookup */
typedef struct
{
  char *reg_name;
  i386_operand_type reg_type;
  unsigned char reg_flags;
#define RegRex	    0x1  /* Extended register.  */
#define RegRex64    0x2  /* Extended 8 bit register.  */
#define RegVRex	    0x4  /* Extended vector register.  */
  unsigned char reg_num;
#define RegRip	((unsigned char ) ~0)
#define RegEip	(RegRip - 1)
/* EIZ and RIZ are fake index registers.  */
#define RegEiz	(RegEip - 1)
#define RegRiz	(RegEiz - 1)
/* FLAT is a fake segment register (Intel mode).  */
#define RegFlat     ((unsigned char) ~0)
  signed char dw2_regnum[2];
#define Dw2Inval (-1)
}
reg_entry;

/* Entries in i386_regtab.  */
#define REGNAM_AL 1
#define REGNAM_AX 25
#define REGNAM_EAX 41

extern const reg_entry i386_regtab[];
extern const unsigned int i386_regtab_size;

typedef struct
{
  char *seg_name;
  unsigned int seg_prefix;
}
seg_entry;

extern const seg_entry cs;
extern const seg_entry ds;
extern const seg_entry ss;
extern const seg_entry es;
extern const seg_entry fs;
extern const seg_entry gs;

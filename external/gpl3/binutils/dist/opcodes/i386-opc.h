/* Declarations for Intel 80386 opcode table
   Copyright 2007, 2008
   Free Software Foundation, Inc.

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

/* i186 or better required */
#define Cpu186		0
/* i286 or better required */
#define Cpu286		(Cpu186 + 1)
/* i386 or better required */
#define Cpu386		(Cpu286 + 1)
/* i486 or better required */
#define Cpu486		(Cpu386 + 1)
/* i585 or better required */
#define Cpu586		(Cpu486 + 1)
/* i686 or better required */
#define Cpu686		(Cpu586 + 1)
/* Pentium4 or better required */
#define CpuP4		(Cpu686 + 1)
/* AMD K6 or better required*/
#define CpuK6		(CpuP4 + 1)
/* AMD K8 or better required */
#define CpuK8		(CpuK6 + 1)
/* MMX support required */
#define CpuMMX		(CpuK8 + 1)
/* SSE support required */
#define CpuSSE		(CpuMMX + 1)
/* SSE2 support required */
#define CpuSSE2		(CpuSSE + 1)
/* 3dnow! support required */
#define Cpu3dnow	(CpuSSE2 + 1)
/* 3dnow! Extensions support required */
#define Cpu3dnowA	(Cpu3dnow + 1)
/* SSE3 support required */
#define CpuSSE3		(Cpu3dnowA + 1)
/* VIA PadLock required */
#define CpuPadLock	(CpuSSE3 + 1)
/* AMD Secure Virtual Machine Ext-s required */
#define CpuSVME		(CpuPadLock + 1)
/* VMX Instructions required */
#define CpuVMX		(CpuSVME + 1)
/* SMX Instructions required */
#define CpuSMX		(CpuVMX + 1)
/* SSSE3 support required */
#define CpuSSSE3	(CpuSMX + 1)
/* SSE4a support required */
#define CpuSSE4a	(CpuSSSE3 + 1)
/* ABM New Instructions required */
#define CpuABM		(CpuSSE4a + 1)
/* SSE4.1 support required */
#define CpuSSE4_1	(CpuABM + 1)
/* SSE4.2 support required */
#define CpuSSE4_2	(CpuSSE4_1 + 1)
/* SSE5 support required */
#define CpuSSE5		(CpuSSE4_2 + 1)
/* AVX support required */
#define CpuAVX		(CpuSSE5 + 1)
/* Xsave/xrstor New Instuctions support required */
#define CpuXsave	(CpuAVX + 1)
/* AES support required */
#define CpuAES		(CpuXsave + 1)
/* PCLMUL support required */
#define CpuPCLMUL	(CpuAES + 1)
/* FMA support required */
#define CpuFMA		(CpuPCLMUL + 1)
/* MOVBE Instuction support required */
#define CpuMovbe	(CpuFMA + 1)
/* EPT Instructions required */
#define CpuEPT		(CpuMovbe + 1)
/* 64bit support available, used by -march= in assembler.  */
#define CpuLM		(CpuEPT + 1)
/* 64bit support required  */
#define Cpu64		(CpuLM + 1)
/* Not supported in the 64bit mode  */
#define CpuNo64		(Cpu64 + 1)
/* The last bitfield in i386_cpu_flags.  */
#define CpuMax		CpuNo64

#define CpuNumOfUints \
  (CpuMax / sizeof (unsigned int) / CHAR_BIT + 1)
#define CpuNumOfBits \
  (CpuNumOfUints * sizeof (unsigned int) * CHAR_BIT)

/* If you get a compiler error for zero width of the unused field,
   comment it out.  */
#define CpuUnused	(CpuMax + 1)

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
      unsigned int cpup4:1;
      unsigned int cpuk6:1;
      unsigned int cpuk8:1;
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
      unsigned int cpusse5:1;
      unsigned int cpuavx:1;
      unsigned int cpuxsave:1;
      unsigned int cpuaes:1;
      unsigned int cpupclmul:1;
      unsigned int cpufma:1;
      unsigned int cpumovbe:1;
      unsigned int cpuept:1;
      unsigned int cpulm:1;
      unsigned int cpu64:1;
      unsigned int cpuno64:1;
#ifdef CpuUnused
      unsigned int unused:(CpuNumOfBits - CpuUnused);
#endif
    } bitfield;
  unsigned int array[CpuNumOfUints];
} i386_cpu_flags;

/* Position of opcode_modifier bits.  */

/* has direction bit. */
#define D			0
/* set if operands can be words or dwords encoded the canonical way */
#define W			(D + 1)
/* insn has a modrm byte. */
#define Modrm			(W + 1)
/* register is in low 3 bits of opcode */
#define ShortForm		(Modrm + 1)
/* special case for jump insns.  */
#define Jump			(ShortForm + 1)
/* call and jump */
#define JumpDword		(Jump + 1)
/* loop and jecxz */
#define JumpByte		(JumpDword + 1)
/* special case for intersegment leaps/calls */
#define JumpInterSegment	(JumpByte + 1)
/* FP insn memory format bit, sized by 0x4 */
#define FloatMF			(JumpInterSegment + 1)
/* src/dest swap for floats. */
#define FloatR			(FloatMF + 1)
/* has float insn direction bit. */
#define FloatD			(FloatR + 1)
/* needs size prefix if in 32-bit mode */
#define Size16			(FloatD + 1)
/* needs size prefix if in 16-bit mode */
#define Size32			(Size16 + 1)
/* needs size prefix if in 64-bit mode */
#define Size64			(Size32 + 1)
/* instruction ignores operand size prefix and in Intel mode ignores
   mnemonic size suffix check.  */
#define IgnoreSize		(Size64 + 1)
/* default insn size depends on mode */
#define DefaultSize		(IgnoreSize + 1)
/* b suffix on instruction illegal */
#define No_bSuf			(DefaultSize + 1)
/* w suffix on instruction illegal */
#define No_wSuf			(No_bSuf + 1)
/* l suffix on instruction illegal */
#define No_lSuf			(No_wSuf + 1)
/* s suffix on instruction illegal */
#define No_sSuf			(No_lSuf + 1)
/* q suffix on instruction illegal */
#define No_qSuf			(No_sSuf + 1)
/* long double suffix on instruction illegal */
#define No_ldSuf		(No_qSuf + 1)
/* instruction needs FWAIT */
#define FWait			(No_ldSuf + 1)
/* quick test for string instructions */
#define IsString		(FWait + 1)
/* fake an extra reg operand for clr, imul and special register
   processing for some instructions.  */
#define RegKludge		(IsString + 1)
/* The first operand must be xmm0 */
#define FirstXmm0		(RegKludge + 1)
/* An implicit xmm0 as the first operand */
#define Implicit1stXmm0		(FirstXmm0 + 1)
/* BYTE is OK in Intel syntax. */
#define ByteOkIntel		(Implicit1stXmm0 + 1)
/* Convert to DWORD */
#define ToDword			(ByteOkIntel + 1)
/* Convert to QWORD */
#define ToQword			(ToDword + 1)
/* Address prefix changes operand 0 */
#define AddrPrefixOp0		(ToQword + 1)
/* opcode is a prefix */
#define IsPrefix		(AddrPrefixOp0 + 1)
/* instruction has extension in 8 bit imm */
#define ImmExt			(IsPrefix + 1)
/* instruction don't need Rex64 prefix.  */
#define NoRex64			(ImmExt + 1)
/* instruction require Rex64 prefix.  */
#define Rex64			(NoRex64 + 1)
/* deprecated fp insn, gets a warning */
#define Ugh			(Rex64 + 1)
#define Drex			(Ugh + 1)
/* instruction needs DREX with multiple encodings for memory ops */
#define Drexv			(Drex + 1)
/* special DREX for comparisons */
#define Drexc			(Drexv + 1)
/* insn has VEX prefix. */
#define Vex			(Drexc + 1)
/* insn has 256bit VEX prefix. */
#define Vex256			(Vex + 1)
/* insn has VEX NDS. Register-only source is encoded in Vex
   prefix. */
#define VexNDS			(Vex256 + 1)
/* insn has VEX NDD. Register destination is encoded in Vex
   prefix. */
#define VexNDD			(VexNDS + 1)
/* insn has VEX W0. */
#define VexW0			(VexNDD + 1)
/* insn has VEX W1. */
#define VexW1			(VexW0 + 1)
/* insn has VEX 0x0F opcode prefix. */
#define Vex0F			(VexW1 + 1)
/* insn has VEX 0x0F38 opcode prefix. */
#define Vex0F38			(Vex0F + 1)
/* insn has VEX 0x0F3A opcode prefix. */
#define Vex0F3A			(Vex0F38 + 1)
/* insn has VEX prefix with 3 soures. */
#define Vex3Sources		(Vex0F3A + 1)
/* instruction has VEX 8 bit imm */
#define VexImmExt		(Vex3Sources + 1)
/* SSE to AVX support required */
#define SSE2AVX			(VexImmExt + 1)
/* No AVX equivalent */
#define NoAVX			(SSE2AVX + 1)
/* Compatible with old (<= 2.8.1) versions of gcc  */
#define OldGcc			(NoAVX + 1)
/* AT&T mnemonic.  */
#define ATTMnemonic		(OldGcc + 1)
/* AT&T syntax.  */
#define ATTSyntax		(ATTMnemonic + 1)
/* Intel syntax.  */
#define IntelSyntax		(ATTSyntax + 1)
/* The last bitfield in i386_opcode_modifier.  */
#define Opcode_Modifier_Max	IntelSyntax

typedef struct i386_opcode_modifier
{
  unsigned int d:1;
  unsigned int w:1;
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
  unsigned int regkludge:1;
  unsigned int firstxmm0:1;
  unsigned int implicit1stxmm0:1;
  unsigned int byteokintel:1;
  unsigned int todword:1;
  unsigned int toqword:1;
  unsigned int addrprefixop0:1;
  unsigned int isprefix:1;
  unsigned int immext:1;
  unsigned int norex64:1;
  unsigned int rex64:1;
  unsigned int ugh:1;
  unsigned int drex:1;
  unsigned int drexv:1;
  unsigned int drexc:1;
  unsigned int vex:1;
  unsigned int vex256:1;
  unsigned int vexnds:1;
  unsigned int vexndd:1;
  unsigned int vexw0:1;
  unsigned int vexw1:1;
  unsigned int vex0f:1;
  unsigned int vex0f38:1;
  unsigned int vex0f3a:1;
  unsigned int vex3sources:1;
  unsigned int veximmext:1;
  unsigned int sse2avx:1;
  unsigned int noavx:1;
  unsigned int oldgcc:1;
  unsigned int attmnemonic:1;
  unsigned int attsyntax:1;
  unsigned int intelsyntax:1;
} i386_opcode_modifier;

/* Position of operand_type bits.  */

/* 8bit register */
#define Reg8			0
/* 16bit register */
#define Reg16			(Reg8 + 1)
/* 32bit register */
#define Reg32			(Reg16 + 1)
/* 64bit register */
#define Reg64			(Reg32 + 1)
/* Floating pointer stack register */
#define FloatReg		(Reg64 + 1)
/* MMX register */
#define RegMMX			(FloatReg + 1)
/* SSE register */
#define RegXMM			(RegMMX + 1)
/* AVX registers */
#define RegYMM			(RegXMM + 1)
/* Control register */
#define Control			(RegYMM + 1)
/* Debug register */
#define Debug			(Control + 1)
/* Test register */
#define Test			(Debug + 1)
/* 2 bit segment register */
#define SReg2			(Test + 1)
/* 3 bit segment register */
#define SReg3			(SReg2 + 1)
/* 1 bit immediate */
#define Imm1			(SReg3 + 1)
/* 8 bit immediate */
#define Imm8			(Imm1 + 1)
/* 8 bit immediate sign extended */
#define Imm8S			(Imm8 + 1)
/* 16 bit immediate */
#define Imm16			(Imm8S + 1)
/* 32 bit immediate */
#define Imm32			(Imm16 + 1)
/* 32 bit immediate sign extended */
#define Imm32S			(Imm32 + 1)
/* 64 bit immediate */
#define Imm64			(Imm32S + 1)
/* 8bit/16bit/32bit displacements are used in different ways,
   depending on the instruction.  For jumps, they specify the
   size of the PC relative displacement, for instructions with
   memory operand, they specify the size of the offset relative
   to the base register, and for instructions with memory offset
   such as `mov 1234,%al' they specify the size of the offset
   relative to the segment base.  */
/* 8 bit displacement */
#define Disp8			(Imm64 + 1)
/* 16 bit displacement */
#define Disp16			(Disp8 + 1)
/* 32 bit displacement */
#define Disp32			(Disp16 + 1)
/* 32 bit signed displacement */
#define Disp32S			(Disp32 + 1)
/* 64 bit displacement */
#define Disp64			(Disp32S + 1)
/* Accumulator %al/%ax/%eax/%rax */
#define Acc			(Disp64 + 1)
/* Floating pointer top stack register %st(0) */
#define FloatAcc		(Acc + 1)
/* Register which can be used for base or index in memory operand.  */
#define BaseIndex		(FloatAcc + 1)
/* Register to hold in/out port addr = dx */
#define InOutPortReg		(BaseIndex + 1)
/* Register to hold shift count = cl */
#define ShiftCount		(InOutPortReg + 1)
/* Absolute address for jump.  */
#define JumpAbsolute		(ShiftCount + 1)
/* String insn operand with fixed es segment */
#define EsSeg			(JumpAbsolute + 1)
/* RegMem is for instructions with a modrm byte where the register
   destination operand should be encoded in the mod and regmem fields.
   Normally, it will be encoded in the reg field. We add a RegMem
   flag to the destination register operand to indicate that it should
   be encoded in the regmem field.  */
#define RegMem			(EsSeg + 1)
/* Memory.  */
#define Mem			(RegMem + 1)
/* BYTE memory. */
#define Byte			(Mem + 1)
/* WORD memory. 2 byte */
#define Word			(Byte + 1)
/* DWORD memory. 4 byte */
#define Dword			(Word + 1)
/* FWORD memory. 6 byte */
#define Fword			(Dword + 1)
/* QWORD memory. 8 byte */
#define Qword			(Fword + 1)
/* TBYTE memory. 10 byte */
#define Tbyte			(Qword + 1)
/* XMMWORD memory. */
#define Xmmword			(Tbyte + 1)
/* YMMWORD memory. */
#define Ymmword			(Xmmword + 1)
/* Unspecified memory size.  */
#define Unspecified		(Ymmword + 1)
/* Any memory size.  */
#define Anysize			(Unspecified  + 1)

/* VEX 4 bit immediate */
#define Vex_Imm4		(Anysize + 1)

/* The last bitfield in i386_operand_type.  */
#define OTMax			Vex_Imm4

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
      unsigned int unspecified:1;
      unsigned int anysize:1;
      unsigned int vex_imm4:1;
#ifdef OTUnused
      unsigned int unused:(OTNumOfBits - OTUnused);
#endif
    } bitfield;
  unsigned int array[OTNumOfUints];
} i386_operand_type;

typedef struct template
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
     Instructions with Drex use this to specify 2 bits for OC */
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
template;

extern const template i386_optab[];

/* these are for register name --> number & type hash lookup */
typedef struct
{
  char *reg_name;
  i386_operand_type reg_type;
  unsigned char reg_flags;
#define RegRex	    0x1  /* Extended register.  */
#define RegRex64    0x2  /* Extended 8 bit register.  */
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

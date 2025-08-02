/* Simulator instruction decoder for bpfbf_ebpfbe.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright (C) 1996-2023 Free Software Foundation, Inc.

This file is part of the GNU simulators.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.

*/

#define WANT_CPU bpfbf
#define WANT_CPU_BPFBF

#include "sim-main.h"
#include "sim-assert.h"
#include "cgen-mem.h"
#include "cgen-ops.h"

/* The instruction descriptor array.
   This is computed at runtime.  Space for it is not malloc'd to save a
   teensy bit of cpu in the decoder.  Moving it to malloc space is trivial
   but won't be done until necessary (we don't currently support the runtime
   addition of instructions nor an SMP machine with different cpus).  */
static IDESC bpfbf_ebpfbe_insn_data[BPFBF_EBPFBE_INSN__MAX];

/* Commas between elements are contained in the macros.
   Some of these are conditionally compiled out.  */

static const struct insn_sem bpfbf_ebpfbe_insn_sem[] =
{
  { VIRTUAL_INSN_X_INVALID, BPFBF_EBPFBE_INSN_X_INVALID, BPFBF_EBPFBE_SFMT_EMPTY },
  { VIRTUAL_INSN_X_AFTER, BPFBF_EBPFBE_INSN_X_AFTER, BPFBF_EBPFBE_SFMT_EMPTY },
  { VIRTUAL_INSN_X_BEFORE, BPFBF_EBPFBE_INSN_X_BEFORE, BPFBF_EBPFBE_SFMT_EMPTY },
  { VIRTUAL_INSN_X_CTI_CHAIN, BPFBF_EBPFBE_INSN_X_CTI_CHAIN, BPFBF_EBPFBE_SFMT_EMPTY },
  { VIRTUAL_INSN_X_CHAIN, BPFBF_EBPFBE_INSN_X_CHAIN, BPFBF_EBPFBE_SFMT_EMPTY },
  { VIRTUAL_INSN_X_BEGIN, BPFBF_EBPFBE_INSN_X_BEGIN, BPFBF_EBPFBE_SFMT_EMPTY },
  { BPF_INSN_ADDIBE, BPFBF_EBPFBE_INSN_ADDIBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_ADDRBE, BPFBF_EBPFBE_INSN_ADDRBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_ADD32IBE, BPFBF_EBPFBE_INSN_ADD32IBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_ADD32RBE, BPFBF_EBPFBE_INSN_ADD32RBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_SUBIBE, BPFBF_EBPFBE_INSN_SUBIBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_SUBRBE, BPFBF_EBPFBE_INSN_SUBRBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_SUB32IBE, BPFBF_EBPFBE_INSN_SUB32IBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_SUB32RBE, BPFBF_EBPFBE_INSN_SUB32RBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_MULIBE, BPFBF_EBPFBE_INSN_MULIBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_MULRBE, BPFBF_EBPFBE_INSN_MULRBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_MUL32IBE, BPFBF_EBPFBE_INSN_MUL32IBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_MUL32RBE, BPFBF_EBPFBE_INSN_MUL32RBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_DIVIBE, BPFBF_EBPFBE_INSN_DIVIBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_DIVRBE, BPFBF_EBPFBE_INSN_DIVRBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_DIV32IBE, BPFBF_EBPFBE_INSN_DIV32IBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_DIV32RBE, BPFBF_EBPFBE_INSN_DIV32RBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_ORIBE, BPFBF_EBPFBE_INSN_ORIBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_ORRBE, BPFBF_EBPFBE_INSN_ORRBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_OR32IBE, BPFBF_EBPFBE_INSN_OR32IBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_OR32RBE, BPFBF_EBPFBE_INSN_OR32RBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_ANDIBE, BPFBF_EBPFBE_INSN_ANDIBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_ANDRBE, BPFBF_EBPFBE_INSN_ANDRBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_AND32IBE, BPFBF_EBPFBE_INSN_AND32IBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_AND32RBE, BPFBF_EBPFBE_INSN_AND32RBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_LSHIBE, BPFBF_EBPFBE_INSN_LSHIBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_LSHRBE, BPFBF_EBPFBE_INSN_LSHRBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_LSH32IBE, BPFBF_EBPFBE_INSN_LSH32IBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_LSH32RBE, BPFBF_EBPFBE_INSN_LSH32RBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_RSHIBE, BPFBF_EBPFBE_INSN_RSHIBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_RSHRBE, BPFBF_EBPFBE_INSN_RSHRBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_RSH32IBE, BPFBF_EBPFBE_INSN_RSH32IBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_RSH32RBE, BPFBF_EBPFBE_INSN_RSH32RBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_MODIBE, BPFBF_EBPFBE_INSN_MODIBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_MODRBE, BPFBF_EBPFBE_INSN_MODRBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_MOD32IBE, BPFBF_EBPFBE_INSN_MOD32IBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_MOD32RBE, BPFBF_EBPFBE_INSN_MOD32RBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_XORIBE, BPFBF_EBPFBE_INSN_XORIBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_XORRBE, BPFBF_EBPFBE_INSN_XORRBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_XOR32IBE, BPFBF_EBPFBE_INSN_XOR32IBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_XOR32RBE, BPFBF_EBPFBE_INSN_XOR32RBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_ARSHIBE, BPFBF_EBPFBE_INSN_ARSHIBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_ARSHRBE, BPFBF_EBPFBE_INSN_ARSHRBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_ARSH32IBE, BPFBF_EBPFBE_INSN_ARSH32IBE, BPFBF_EBPFBE_SFMT_ADDIBE },
  { BPF_INSN_ARSH32RBE, BPFBF_EBPFBE_INSN_ARSH32RBE, BPFBF_EBPFBE_SFMT_ADDRBE },
  { BPF_INSN_NEGBE, BPFBF_EBPFBE_INSN_NEGBE, BPFBF_EBPFBE_SFMT_NEGBE },
  { BPF_INSN_NEG32BE, BPFBF_EBPFBE_INSN_NEG32BE, BPFBF_EBPFBE_SFMT_NEGBE },
  { BPF_INSN_MOVIBE, BPFBF_EBPFBE_INSN_MOVIBE, BPFBF_EBPFBE_SFMT_MOVIBE },
  { BPF_INSN_MOVRBE, BPFBF_EBPFBE_INSN_MOVRBE, BPFBF_EBPFBE_SFMT_MOVRBE },
  { BPF_INSN_MOV32IBE, BPFBF_EBPFBE_INSN_MOV32IBE, BPFBF_EBPFBE_SFMT_MOVIBE },
  { BPF_INSN_MOV32RBE, BPFBF_EBPFBE_INSN_MOV32RBE, BPFBF_EBPFBE_SFMT_MOVRBE },
  { BPF_INSN_ENDLEBE, BPFBF_EBPFBE_INSN_ENDLEBE, BPFBF_EBPFBE_SFMT_ENDLEBE },
  { BPF_INSN_ENDBEBE, BPFBF_EBPFBE_INSN_ENDBEBE, BPFBF_EBPFBE_SFMT_ENDLEBE },
  { BPF_INSN_LDDWBE, BPFBF_EBPFBE_INSN_LDDWBE, BPFBF_EBPFBE_SFMT_LDDWBE },
  { BPF_INSN_LDABSW, BPFBF_EBPFBE_INSN_LDABSW, BPFBF_EBPFBE_SFMT_LDABSW },
  { BPF_INSN_LDABSH, BPFBF_EBPFBE_INSN_LDABSH, BPFBF_EBPFBE_SFMT_LDABSH },
  { BPF_INSN_LDABSB, BPFBF_EBPFBE_INSN_LDABSB, BPFBF_EBPFBE_SFMT_LDABSB },
  { BPF_INSN_LDABSDW, BPFBF_EBPFBE_INSN_LDABSDW, BPFBF_EBPFBE_SFMT_LDABSDW },
  { BPF_INSN_LDINDWBE, BPFBF_EBPFBE_INSN_LDINDWBE, BPFBF_EBPFBE_SFMT_LDINDWBE },
  { BPF_INSN_LDINDHBE, BPFBF_EBPFBE_INSN_LDINDHBE, BPFBF_EBPFBE_SFMT_LDINDHBE },
  { BPF_INSN_LDINDBBE, BPFBF_EBPFBE_INSN_LDINDBBE, BPFBF_EBPFBE_SFMT_LDINDBBE },
  { BPF_INSN_LDINDDWBE, BPFBF_EBPFBE_INSN_LDINDDWBE, BPFBF_EBPFBE_SFMT_LDINDDWBE },
  { BPF_INSN_LDXWBE, BPFBF_EBPFBE_INSN_LDXWBE, BPFBF_EBPFBE_SFMT_LDXWBE },
  { BPF_INSN_LDXHBE, BPFBF_EBPFBE_INSN_LDXHBE, BPFBF_EBPFBE_SFMT_LDXHBE },
  { BPF_INSN_LDXBBE, BPFBF_EBPFBE_INSN_LDXBBE, BPFBF_EBPFBE_SFMT_LDXBBE },
  { BPF_INSN_LDXDWBE, BPFBF_EBPFBE_INSN_LDXDWBE, BPFBF_EBPFBE_SFMT_LDXDWBE },
  { BPF_INSN_STXWBE, BPFBF_EBPFBE_INSN_STXWBE, BPFBF_EBPFBE_SFMT_STXWBE },
  { BPF_INSN_STXHBE, BPFBF_EBPFBE_INSN_STXHBE, BPFBF_EBPFBE_SFMT_STXHBE },
  { BPF_INSN_STXBBE, BPFBF_EBPFBE_INSN_STXBBE, BPFBF_EBPFBE_SFMT_STXBBE },
  { BPF_INSN_STXDWBE, BPFBF_EBPFBE_INSN_STXDWBE, BPFBF_EBPFBE_SFMT_STXDWBE },
  { BPF_INSN_STBBE, BPFBF_EBPFBE_INSN_STBBE, BPFBF_EBPFBE_SFMT_STBBE },
  { BPF_INSN_STHBE, BPFBF_EBPFBE_INSN_STHBE, BPFBF_EBPFBE_SFMT_STHBE },
  { BPF_INSN_STWBE, BPFBF_EBPFBE_INSN_STWBE, BPFBF_EBPFBE_SFMT_STWBE },
  { BPF_INSN_STDWBE, BPFBF_EBPFBE_INSN_STDWBE, BPFBF_EBPFBE_SFMT_STDWBE },
  { BPF_INSN_JEQIBE, BPFBF_EBPFBE_INSN_JEQIBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JEQRBE, BPFBF_EBPFBE_INSN_JEQRBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_JEQ32IBE, BPFBF_EBPFBE_INSN_JEQ32IBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JEQ32RBE, BPFBF_EBPFBE_INSN_JEQ32RBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_JGTIBE, BPFBF_EBPFBE_INSN_JGTIBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JGTRBE, BPFBF_EBPFBE_INSN_JGTRBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_JGT32IBE, BPFBF_EBPFBE_INSN_JGT32IBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JGT32RBE, BPFBF_EBPFBE_INSN_JGT32RBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_JGEIBE, BPFBF_EBPFBE_INSN_JGEIBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JGERBE, BPFBF_EBPFBE_INSN_JGERBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_JGE32IBE, BPFBF_EBPFBE_INSN_JGE32IBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JGE32RBE, BPFBF_EBPFBE_INSN_JGE32RBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_JLTIBE, BPFBF_EBPFBE_INSN_JLTIBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JLTRBE, BPFBF_EBPFBE_INSN_JLTRBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_JLT32IBE, BPFBF_EBPFBE_INSN_JLT32IBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JLT32RBE, BPFBF_EBPFBE_INSN_JLT32RBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_JLEIBE, BPFBF_EBPFBE_INSN_JLEIBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JLERBE, BPFBF_EBPFBE_INSN_JLERBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_JLE32IBE, BPFBF_EBPFBE_INSN_JLE32IBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JLE32RBE, BPFBF_EBPFBE_INSN_JLE32RBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_JSETIBE, BPFBF_EBPFBE_INSN_JSETIBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JSETRBE, BPFBF_EBPFBE_INSN_JSETRBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_JSET32IBE, BPFBF_EBPFBE_INSN_JSET32IBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JSET32RBE, BPFBF_EBPFBE_INSN_JSET32RBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_JNEIBE, BPFBF_EBPFBE_INSN_JNEIBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JNERBE, BPFBF_EBPFBE_INSN_JNERBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_JNE32IBE, BPFBF_EBPFBE_INSN_JNE32IBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JNE32RBE, BPFBF_EBPFBE_INSN_JNE32RBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_JSGTIBE, BPFBF_EBPFBE_INSN_JSGTIBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JSGTRBE, BPFBF_EBPFBE_INSN_JSGTRBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_JSGT32IBE, BPFBF_EBPFBE_INSN_JSGT32IBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JSGT32RBE, BPFBF_EBPFBE_INSN_JSGT32RBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_JSGEIBE, BPFBF_EBPFBE_INSN_JSGEIBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JSGERBE, BPFBF_EBPFBE_INSN_JSGERBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_JSGE32IBE, BPFBF_EBPFBE_INSN_JSGE32IBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JSGE32RBE, BPFBF_EBPFBE_INSN_JSGE32RBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_JSLTIBE, BPFBF_EBPFBE_INSN_JSLTIBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JSLTRBE, BPFBF_EBPFBE_INSN_JSLTRBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_JSLT32IBE, BPFBF_EBPFBE_INSN_JSLT32IBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JSLT32RBE, BPFBF_EBPFBE_INSN_JSLT32RBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_JSLEIBE, BPFBF_EBPFBE_INSN_JSLEIBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JSLERBE, BPFBF_EBPFBE_INSN_JSLERBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_JSLE32IBE, BPFBF_EBPFBE_INSN_JSLE32IBE, BPFBF_EBPFBE_SFMT_JEQIBE },
  { BPF_INSN_JSLE32RBE, BPFBF_EBPFBE_INSN_JSLE32RBE, BPFBF_EBPFBE_SFMT_JEQRBE },
  { BPF_INSN_CALLBE, BPFBF_EBPFBE_INSN_CALLBE, BPFBF_EBPFBE_SFMT_CALLBE },
  { BPF_INSN_JA, BPFBF_EBPFBE_INSN_JA, BPFBF_EBPFBE_SFMT_JA },
  { BPF_INSN_EXIT, BPFBF_EBPFBE_INSN_EXIT, BPFBF_EBPFBE_SFMT_EXIT },
  { BPF_INSN_XADDDWBE, BPFBF_EBPFBE_INSN_XADDDWBE, BPFBF_EBPFBE_SFMT_XADDDWBE },
  { BPF_INSN_XADDWBE, BPFBF_EBPFBE_INSN_XADDWBE, BPFBF_EBPFBE_SFMT_XADDWBE },
  { BPF_INSN_BRKPT, BPFBF_EBPFBE_INSN_BRKPT, BPFBF_EBPFBE_SFMT_EXIT },
};

static const struct insn_sem bpfbf_ebpfbe_insn_sem_invalid =
{
  VIRTUAL_INSN_X_INVALID, BPFBF_EBPFBE_INSN_X_INVALID, BPFBF_EBPFBE_SFMT_EMPTY
};

/* Initialize an IDESC from the compile-time computable parts.  */

static INLINE void
init_idesc (SIM_CPU *cpu, IDESC *id, const struct insn_sem *t)
{
  const CGEN_INSN *insn_table = CGEN_CPU_INSN_TABLE (CPU_CPU_DESC (cpu))->init_entries;

  id->num = t->index;
  id->sfmt = t->sfmt;
  if ((int) t->type <= 0)
    id->idata = & cgen_virtual_insn_table[- (int) t->type];
  else
    id->idata = & insn_table[t->type];
  id->attrs = CGEN_INSN_ATTRS (id->idata);
  /* Oh my god, a magic number.  */
  id->length = CGEN_INSN_BITSIZE (id->idata) / 8;

#if WITH_PROFILE_MODEL_P
  id->timing = & MODEL_TIMING (CPU_MODEL (cpu)) [t->index];
  {
    SIM_DESC sd = CPU_STATE (cpu);
    SIM_ASSERT (t->index == id->timing->num);
  }
#endif

  /* Semantic pointers are initialized elsewhere.  */
}

/* Initialize the instruction descriptor table.  */

void
bpfbf_ebpfbe_init_idesc_table (SIM_CPU *cpu)
{
  IDESC *id,*tabend;
  const struct insn_sem *t,*tend;
  int tabsize = BPFBF_EBPFBE_INSN__MAX;
  IDESC *table = bpfbf_ebpfbe_insn_data;

  memset (table, 0, tabsize * sizeof (IDESC));

  /* First set all entries to the `invalid insn'.  */
  t = & bpfbf_ebpfbe_insn_sem_invalid;
  for (id = table, tabend = table + tabsize; id < tabend; ++id)
    init_idesc (cpu, id, t);

  /* Now fill in the values for the chosen cpu.  */
  for (t = bpfbf_ebpfbe_insn_sem, tend = t + sizeof (bpfbf_ebpfbe_insn_sem) / sizeof (*t);
       t != tend; ++t)
    {
      init_idesc (cpu, & table[t->index], t);
    }

  /* Link the IDESC table into the cpu.  */
  CPU_IDESC (cpu) = table;
}

/* Given an instruction, return a pointer to its IDESC entry.  */

const IDESC *
bpfbf_ebpfbe_decode (SIM_CPU *current_cpu, IADDR pc,
              CGEN_INSN_WORD base_insn,
              ARGBUF *abuf)
{
  /* Result of decoder.  */
  BPFBF_EBPFBE_INSN_TYPE itype;

  {
    CGEN_INSN_WORD insn = base_insn;

    {
      unsigned int val = (((insn >> 0) & (255 << 0)));
      switch (val)
      {
      case 4 : itype = BPFBF_EBPFBE_INSN_ADD32IBE; goto extract_sfmt_addibe;
      case 5 : itype = BPFBF_EBPFBE_INSN_JA; goto extract_sfmt_ja;
      case 7 : itype = BPFBF_EBPFBE_INSN_ADDIBE; goto extract_sfmt_addibe;
      case 12 : itype = BPFBF_EBPFBE_INSN_ADD32RBE; goto extract_sfmt_addrbe;
      case 15 : itype = BPFBF_EBPFBE_INSN_ADDRBE; goto extract_sfmt_addrbe;
      case 20 : itype = BPFBF_EBPFBE_INSN_SUB32IBE; goto extract_sfmt_addibe;
      case 21 : itype = BPFBF_EBPFBE_INSN_JEQIBE; goto extract_sfmt_jeqibe;
      case 22 : itype = BPFBF_EBPFBE_INSN_JEQ32IBE; goto extract_sfmt_jeqibe;
      case 23 : itype = BPFBF_EBPFBE_INSN_SUBIBE; goto extract_sfmt_addibe;
      case 24 : itype = BPFBF_EBPFBE_INSN_LDDWBE; goto extract_sfmt_lddwbe;
      case 28 : itype = BPFBF_EBPFBE_INSN_SUB32RBE; goto extract_sfmt_addrbe;
      case 29 : itype = BPFBF_EBPFBE_INSN_JEQRBE; goto extract_sfmt_jeqrbe;
      case 30 : itype = BPFBF_EBPFBE_INSN_JEQ32RBE; goto extract_sfmt_jeqrbe;
      case 31 : itype = BPFBF_EBPFBE_INSN_SUBRBE; goto extract_sfmt_addrbe;
      case 32 : itype = BPFBF_EBPFBE_INSN_LDABSW; goto extract_sfmt_ldabsw;
      case 36 : itype = BPFBF_EBPFBE_INSN_MUL32IBE; goto extract_sfmt_addibe;
      case 37 : itype = BPFBF_EBPFBE_INSN_JGTIBE; goto extract_sfmt_jeqibe;
      case 38 : itype = BPFBF_EBPFBE_INSN_JGT32IBE; goto extract_sfmt_jeqibe;
      case 39 : itype = BPFBF_EBPFBE_INSN_MULIBE; goto extract_sfmt_addibe;
      case 40 : itype = BPFBF_EBPFBE_INSN_LDABSH; goto extract_sfmt_ldabsh;
      case 44 : itype = BPFBF_EBPFBE_INSN_MUL32RBE; goto extract_sfmt_addrbe;
      case 45 : itype = BPFBF_EBPFBE_INSN_JGTRBE; goto extract_sfmt_jeqrbe;
      case 46 : itype = BPFBF_EBPFBE_INSN_JGT32RBE; goto extract_sfmt_jeqrbe;
      case 47 : itype = BPFBF_EBPFBE_INSN_MULRBE; goto extract_sfmt_addrbe;
      case 48 : itype = BPFBF_EBPFBE_INSN_LDABSB; goto extract_sfmt_ldabsb;
      case 52 : itype = BPFBF_EBPFBE_INSN_DIV32IBE; goto extract_sfmt_addibe;
      case 53 : itype = BPFBF_EBPFBE_INSN_JGEIBE; goto extract_sfmt_jeqibe;
      case 54 : itype = BPFBF_EBPFBE_INSN_JGE32IBE; goto extract_sfmt_jeqibe;
      case 55 : itype = BPFBF_EBPFBE_INSN_DIVIBE; goto extract_sfmt_addibe;
      case 56 : itype = BPFBF_EBPFBE_INSN_LDABSDW; goto extract_sfmt_ldabsdw;
      case 60 : itype = BPFBF_EBPFBE_INSN_DIV32RBE; goto extract_sfmt_addrbe;
      case 61 : itype = BPFBF_EBPFBE_INSN_JGERBE; goto extract_sfmt_jeqrbe;
      case 62 : itype = BPFBF_EBPFBE_INSN_JGE32RBE; goto extract_sfmt_jeqrbe;
      case 63 : itype = BPFBF_EBPFBE_INSN_DIVRBE; goto extract_sfmt_addrbe;
      case 64 : itype = BPFBF_EBPFBE_INSN_LDINDWBE; goto extract_sfmt_ldindwbe;
      case 68 : itype = BPFBF_EBPFBE_INSN_OR32IBE; goto extract_sfmt_addibe;
      case 69 : itype = BPFBF_EBPFBE_INSN_JSETIBE; goto extract_sfmt_jeqibe;
      case 70 : itype = BPFBF_EBPFBE_INSN_JSET32IBE; goto extract_sfmt_jeqibe;
      case 71 : itype = BPFBF_EBPFBE_INSN_ORIBE; goto extract_sfmt_addibe;
      case 72 : itype = BPFBF_EBPFBE_INSN_LDINDHBE; goto extract_sfmt_ldindhbe;
      case 76 : itype = BPFBF_EBPFBE_INSN_OR32RBE; goto extract_sfmt_addrbe;
      case 77 : itype = BPFBF_EBPFBE_INSN_JSETRBE; goto extract_sfmt_jeqrbe;
      case 78 : itype = BPFBF_EBPFBE_INSN_JSET32RBE; goto extract_sfmt_jeqrbe;
      case 79 : itype = BPFBF_EBPFBE_INSN_ORRBE; goto extract_sfmt_addrbe;
      case 80 : itype = BPFBF_EBPFBE_INSN_LDINDBBE; goto extract_sfmt_ldindbbe;
      case 84 : itype = BPFBF_EBPFBE_INSN_AND32IBE; goto extract_sfmt_addibe;
      case 85 : itype = BPFBF_EBPFBE_INSN_JNEIBE; goto extract_sfmt_jeqibe;
      case 86 : itype = BPFBF_EBPFBE_INSN_JNE32IBE; goto extract_sfmt_jeqibe;
      case 87 : itype = BPFBF_EBPFBE_INSN_ANDIBE; goto extract_sfmt_addibe;
      case 88 : itype = BPFBF_EBPFBE_INSN_LDINDDWBE; goto extract_sfmt_ldinddwbe;
      case 92 : itype = BPFBF_EBPFBE_INSN_AND32RBE; goto extract_sfmt_addrbe;
      case 93 : itype = BPFBF_EBPFBE_INSN_JNERBE; goto extract_sfmt_jeqrbe;
      case 94 : itype = BPFBF_EBPFBE_INSN_JNE32RBE; goto extract_sfmt_jeqrbe;
      case 95 : itype = BPFBF_EBPFBE_INSN_ANDRBE; goto extract_sfmt_addrbe;
      case 97 : itype = BPFBF_EBPFBE_INSN_LDXWBE; goto extract_sfmt_ldxwbe;
      case 98 : itype = BPFBF_EBPFBE_INSN_STWBE; goto extract_sfmt_stwbe;
      case 99 : itype = BPFBF_EBPFBE_INSN_STXWBE; goto extract_sfmt_stxwbe;
      case 100 : itype = BPFBF_EBPFBE_INSN_LSH32IBE; goto extract_sfmt_addibe;
      case 101 : itype = BPFBF_EBPFBE_INSN_JSGTIBE; goto extract_sfmt_jeqibe;
      case 102 : itype = BPFBF_EBPFBE_INSN_JSGT32IBE; goto extract_sfmt_jeqibe;
      case 103 : itype = BPFBF_EBPFBE_INSN_LSHIBE; goto extract_sfmt_addibe;
      case 105 : itype = BPFBF_EBPFBE_INSN_LDXHBE; goto extract_sfmt_ldxhbe;
      case 106 : itype = BPFBF_EBPFBE_INSN_STHBE; goto extract_sfmt_sthbe;
      case 107 : itype = BPFBF_EBPFBE_INSN_STXHBE; goto extract_sfmt_stxhbe;
      case 108 : itype = BPFBF_EBPFBE_INSN_LSH32RBE; goto extract_sfmt_addrbe;
      case 109 : itype = BPFBF_EBPFBE_INSN_JSGTRBE; goto extract_sfmt_jeqrbe;
      case 110 : itype = BPFBF_EBPFBE_INSN_JSGT32RBE; goto extract_sfmt_jeqrbe;
      case 111 : itype = BPFBF_EBPFBE_INSN_LSHRBE; goto extract_sfmt_addrbe;
      case 113 : itype = BPFBF_EBPFBE_INSN_LDXBBE; goto extract_sfmt_ldxbbe;
      case 114 : itype = BPFBF_EBPFBE_INSN_STBBE; goto extract_sfmt_stbbe;
      case 115 : itype = BPFBF_EBPFBE_INSN_STXBBE; goto extract_sfmt_stxbbe;
      case 116 : itype = BPFBF_EBPFBE_INSN_RSH32IBE; goto extract_sfmt_addibe;
      case 117 : itype = BPFBF_EBPFBE_INSN_JSGEIBE; goto extract_sfmt_jeqibe;
      case 118 : itype = BPFBF_EBPFBE_INSN_JSGE32IBE; goto extract_sfmt_jeqibe;
      case 119 : itype = BPFBF_EBPFBE_INSN_RSHIBE; goto extract_sfmt_addibe;
      case 121 : itype = BPFBF_EBPFBE_INSN_LDXDWBE; goto extract_sfmt_ldxdwbe;
      case 122 : itype = BPFBF_EBPFBE_INSN_STDWBE; goto extract_sfmt_stdwbe;
      case 123 : itype = BPFBF_EBPFBE_INSN_STXDWBE; goto extract_sfmt_stxdwbe;
      case 124 : itype = BPFBF_EBPFBE_INSN_RSH32RBE; goto extract_sfmt_addrbe;
      case 125 : itype = BPFBF_EBPFBE_INSN_JSGERBE; goto extract_sfmt_jeqrbe;
      case 126 : itype = BPFBF_EBPFBE_INSN_JSGE32RBE; goto extract_sfmt_jeqrbe;
      case 127 : itype = BPFBF_EBPFBE_INSN_RSHRBE; goto extract_sfmt_addrbe;
      case 132 : itype = BPFBF_EBPFBE_INSN_NEG32BE; goto extract_sfmt_negbe;
      case 133 : itype = BPFBF_EBPFBE_INSN_CALLBE; goto extract_sfmt_callbe;
      case 135 : itype = BPFBF_EBPFBE_INSN_NEGBE; goto extract_sfmt_negbe;
      case 140 : itype = BPFBF_EBPFBE_INSN_BRKPT; goto extract_sfmt_exit;
      case 148 : itype = BPFBF_EBPFBE_INSN_MOD32IBE; goto extract_sfmt_addibe;
      case 149 : itype = BPFBF_EBPFBE_INSN_EXIT; goto extract_sfmt_exit;
      case 151 : itype = BPFBF_EBPFBE_INSN_MODIBE; goto extract_sfmt_addibe;
      case 156 : itype = BPFBF_EBPFBE_INSN_MOD32RBE; goto extract_sfmt_addrbe;
      case 159 : itype = BPFBF_EBPFBE_INSN_MODRBE; goto extract_sfmt_addrbe;
      case 164 : itype = BPFBF_EBPFBE_INSN_XOR32IBE; goto extract_sfmt_addibe;
      case 165 : itype = BPFBF_EBPFBE_INSN_JLTIBE; goto extract_sfmt_jeqibe;
      case 166 : itype = BPFBF_EBPFBE_INSN_JLT32IBE; goto extract_sfmt_jeqibe;
      case 167 : itype = BPFBF_EBPFBE_INSN_XORIBE; goto extract_sfmt_addibe;
      case 172 : itype = BPFBF_EBPFBE_INSN_XOR32RBE; goto extract_sfmt_addrbe;
      case 173 : itype = BPFBF_EBPFBE_INSN_JLTRBE; goto extract_sfmt_jeqrbe;
      case 174 : itype = BPFBF_EBPFBE_INSN_JLT32RBE; goto extract_sfmt_jeqrbe;
      case 175 : itype = BPFBF_EBPFBE_INSN_XORRBE; goto extract_sfmt_addrbe;
      case 180 : itype = BPFBF_EBPFBE_INSN_MOV32IBE; goto extract_sfmt_movibe;
      case 181 : itype = BPFBF_EBPFBE_INSN_JLEIBE; goto extract_sfmt_jeqibe;
      case 182 : itype = BPFBF_EBPFBE_INSN_JLE32IBE; goto extract_sfmt_jeqibe;
      case 183 : itype = BPFBF_EBPFBE_INSN_MOVIBE; goto extract_sfmt_movibe;
      case 188 : itype = BPFBF_EBPFBE_INSN_MOV32RBE; goto extract_sfmt_movrbe;
      case 189 : itype = BPFBF_EBPFBE_INSN_JLERBE; goto extract_sfmt_jeqrbe;
      case 190 : itype = BPFBF_EBPFBE_INSN_JLE32RBE; goto extract_sfmt_jeqrbe;
      case 191 : itype = BPFBF_EBPFBE_INSN_MOVRBE; goto extract_sfmt_movrbe;
      case 195 : itype = BPFBF_EBPFBE_INSN_XADDWBE; goto extract_sfmt_xaddwbe;
      case 196 : itype = BPFBF_EBPFBE_INSN_ARSH32IBE; goto extract_sfmt_addibe;
      case 197 : itype = BPFBF_EBPFBE_INSN_JSLTIBE; goto extract_sfmt_jeqibe;
      case 198 : itype = BPFBF_EBPFBE_INSN_JSLT32IBE; goto extract_sfmt_jeqibe;
      case 199 : itype = BPFBF_EBPFBE_INSN_ARSHIBE; goto extract_sfmt_addibe;
      case 204 : itype = BPFBF_EBPFBE_INSN_ARSH32RBE; goto extract_sfmt_addrbe;
      case 205 : itype = BPFBF_EBPFBE_INSN_JSLTRBE; goto extract_sfmt_jeqrbe;
      case 206 : itype = BPFBF_EBPFBE_INSN_JSLT32RBE; goto extract_sfmt_jeqrbe;
      case 207 : itype = BPFBF_EBPFBE_INSN_ARSHRBE; goto extract_sfmt_addrbe;
      case 212 : itype = BPFBF_EBPFBE_INSN_ENDLEBE; goto extract_sfmt_endlebe;
      case 213 : itype = BPFBF_EBPFBE_INSN_JSLEIBE; goto extract_sfmt_jeqibe;
      case 214 : itype = BPFBF_EBPFBE_INSN_JSLE32IBE; goto extract_sfmt_jeqibe;
      case 219 : itype = BPFBF_EBPFBE_INSN_XADDDWBE; goto extract_sfmt_xadddwbe;
      case 220 : itype = BPFBF_EBPFBE_INSN_ENDBEBE; goto extract_sfmt_endlebe;
      case 221 : itype = BPFBF_EBPFBE_INSN_JSLERBE; goto extract_sfmt_jeqrbe;
      case 222 : itype = BPFBF_EBPFBE_INSN_JSLE32RBE; goto extract_sfmt_jeqrbe;
      default : itype = BPFBF_EBPFBE_INSN_X_INVALID; goto extract_sfmt_empty;
      }
    }
  }

  /* The instruction has been decoded, now extract the fields.  */

 extract_sfmt_empty:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_empty.f


  /* Record the fields for the semantic handler.  */
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_empty", (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_addibe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_stbbe.f
    INT f_imm32;
    UINT f_dstbe;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstbe) = f_dstbe;
  FLD (f_imm32) = f_imm32;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addibe", "f_dstbe 0x%x", 'x', f_dstbe, "f_imm32 0x%x", 'x', f_imm32, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_addrbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
    UINT f_dstbe;
    UINT f_srcbe;

    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_srcbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstbe) = f_dstbe;
  FLD (f_srcbe) = f_srcbe;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addrbe", "f_dstbe 0x%x", 'x', f_dstbe, "f_srcbe 0x%x", 'x', f_srcbe, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_negbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_lddwbe.f
    UINT f_dstbe;

    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstbe) = f_dstbe;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_negbe", "f_dstbe 0x%x", 'x', f_dstbe, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_movibe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_stbbe.f
    INT f_imm32;
    UINT f_dstbe;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_imm32) = f_imm32;
  FLD (f_dstbe) = f_dstbe;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_movibe", "f_imm32 0x%x", 'x', f_imm32, "f_dstbe 0x%x", 'x', f_dstbe, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_movrbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
    UINT f_dstbe;
    UINT f_srcbe;

    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_srcbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_srcbe) = f_srcbe;
  FLD (f_dstbe) = f_dstbe;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_movrbe", "f_srcbe 0x%x", 'x', f_srcbe, "f_dstbe 0x%x", 'x', f_dstbe, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_endlebe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_stbbe.f
    INT f_imm32;
    UINT f_dstbe;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstbe) = f_dstbe;
  FLD (f_imm32) = f_imm32;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_endlebe", "f_dstbe 0x%x", 'x', f_dstbe, "f_imm32 0x%x", 'x', f_imm32, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lddwbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_lddwbe.f
    UINT f_imm64_c;
    UINT f_imm64_b;
    UINT f_imm64_a;
    UINT f_dstbe;
    DI f_imm64;
    /* Contents of trailing part of insn.  */
    UINT word_1;
    UINT word_2;

  word_1 = GETIMEMUSI (current_cpu, pc + 8);
  word_2 = GETIMEMUSI (current_cpu, pc + 12);
    f_imm64_c = (0|(EXTRACT_LSB0_UINT (word_2, 32, 31, 32) << 0));
    f_imm64_b = (0|(EXTRACT_LSB0_UINT (word_1, 32, 31, 32) << 0));
    f_imm64_a = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
{
  f_imm64 = ((((((UDI) (UINT) (f_imm64_c))) << (32))) | (((UDI) (UINT) (f_imm64_a))));
}

  /* Record the fields for the semantic handler.  */
  FLD (f_imm64) = f_imm64;
  FLD (f_dstbe) = f_dstbe;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lddwbe", "f_imm64 0x%x", 'x', f_imm64, "f_dstbe 0x%x", 'x', f_dstbe, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldabsw:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldindwbe.f
    INT f_imm32;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_imm32) = f_imm32;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldabsw", "f_imm32 0x%x", 'x', f_imm32, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldabsh:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldindwbe.f
    INT f_imm32;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_imm32) = f_imm32;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldabsh", "f_imm32 0x%x", 'x', f_imm32, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldabsb:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldindwbe.f
    INT f_imm32;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_imm32) = f_imm32;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldabsb", "f_imm32 0x%x", 'x', f_imm32, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldabsdw:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldindwbe.f
    INT f_imm32;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_imm32) = f_imm32;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldabsdw", "f_imm32 0x%x", 'x', f_imm32, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldindwbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldindwbe.f
    INT f_imm32;
    UINT f_srcbe;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_srcbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_imm32) = f_imm32;
  FLD (f_srcbe) = f_srcbe;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldindwbe", "f_imm32 0x%x", 'x', f_imm32, "f_srcbe 0x%x", 'x', f_srcbe, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldindhbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldindwbe.f
    INT f_imm32;
    UINT f_srcbe;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_srcbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_imm32) = f_imm32;
  FLD (f_srcbe) = f_srcbe;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldindhbe", "f_imm32 0x%x", 'x', f_imm32, "f_srcbe 0x%x", 'x', f_srcbe, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldindbbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldindwbe.f
    INT f_imm32;
    UINT f_srcbe;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_srcbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_imm32) = f_imm32;
  FLD (f_srcbe) = f_srcbe;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldindbbe", "f_imm32 0x%x", 'x', f_imm32, "f_srcbe 0x%x", 'x', f_srcbe, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldinddwbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldindwbe.f
    INT f_imm32;
    UINT f_srcbe;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_srcbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_imm32) = f_imm32;
  FLD (f_srcbe) = f_srcbe;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldinddwbe", "f_imm32 0x%x", 'x', f_imm32, "f_srcbe 0x%x", 'x', f_srcbe, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldxwbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
    HI f_offset16;
    UINT f_dstbe;
    UINT f_srcbe;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_srcbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_offset16) = f_offset16;
  FLD (f_srcbe) = f_srcbe;
  FLD (f_dstbe) = f_dstbe;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldxwbe", "f_offset16 0x%x", 'x', f_offset16, "f_srcbe 0x%x", 'x', f_srcbe, "f_dstbe 0x%x", 'x', f_dstbe, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldxhbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
    HI f_offset16;
    UINT f_dstbe;
    UINT f_srcbe;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_srcbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_offset16) = f_offset16;
  FLD (f_srcbe) = f_srcbe;
  FLD (f_dstbe) = f_dstbe;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldxhbe", "f_offset16 0x%x", 'x', f_offset16, "f_srcbe 0x%x", 'x', f_srcbe, "f_dstbe 0x%x", 'x', f_dstbe, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldxbbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
    HI f_offset16;
    UINT f_dstbe;
    UINT f_srcbe;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_srcbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_offset16) = f_offset16;
  FLD (f_srcbe) = f_srcbe;
  FLD (f_dstbe) = f_dstbe;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldxbbe", "f_offset16 0x%x", 'x', f_offset16, "f_srcbe 0x%x", 'x', f_srcbe, "f_dstbe 0x%x", 'x', f_dstbe, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldxdwbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
    HI f_offset16;
    UINT f_dstbe;
    UINT f_srcbe;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_srcbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_offset16) = f_offset16;
  FLD (f_srcbe) = f_srcbe;
  FLD (f_dstbe) = f_dstbe;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldxdwbe", "f_offset16 0x%x", 'x', f_offset16, "f_srcbe 0x%x", 'x', f_srcbe, "f_dstbe 0x%x", 'x', f_dstbe, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_stxwbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
    HI f_offset16;
    UINT f_dstbe;
    UINT f_srcbe;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_srcbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstbe) = f_dstbe;
  FLD (f_offset16) = f_offset16;
  FLD (f_srcbe) = f_srcbe;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stxwbe", "f_dstbe 0x%x", 'x', f_dstbe, "f_offset16 0x%x", 'x', f_offset16, "f_srcbe 0x%x", 'x', f_srcbe, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_stxhbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
    HI f_offset16;
    UINT f_dstbe;
    UINT f_srcbe;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_srcbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstbe) = f_dstbe;
  FLD (f_offset16) = f_offset16;
  FLD (f_srcbe) = f_srcbe;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stxhbe", "f_dstbe 0x%x", 'x', f_dstbe, "f_offset16 0x%x", 'x', f_offset16, "f_srcbe 0x%x", 'x', f_srcbe, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_stxbbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
    HI f_offset16;
    UINT f_dstbe;
    UINT f_srcbe;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_srcbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstbe) = f_dstbe;
  FLD (f_offset16) = f_offset16;
  FLD (f_srcbe) = f_srcbe;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stxbbe", "f_dstbe 0x%x", 'x', f_dstbe, "f_offset16 0x%x", 'x', f_offset16, "f_srcbe 0x%x", 'x', f_srcbe, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_stxdwbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
    HI f_offset16;
    UINT f_dstbe;
    UINT f_srcbe;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_srcbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstbe) = f_dstbe;
  FLD (f_offset16) = f_offset16;
  FLD (f_srcbe) = f_srcbe;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stxdwbe", "f_dstbe 0x%x", 'x', f_dstbe, "f_offset16 0x%x", 'x', f_offset16, "f_srcbe 0x%x", 'x', f_srcbe, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_stbbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_stbbe.f
    INT f_imm32;
    HI f_offset16;
    UINT f_dstbe;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstbe) = f_dstbe;
  FLD (f_imm32) = f_imm32;
  FLD (f_offset16) = f_offset16;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stbbe", "f_dstbe 0x%x", 'x', f_dstbe, "f_imm32 0x%x", 'x', f_imm32, "f_offset16 0x%x", 'x', f_offset16, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_sthbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_stbbe.f
    INT f_imm32;
    HI f_offset16;
    UINT f_dstbe;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstbe) = f_dstbe;
  FLD (f_imm32) = f_imm32;
  FLD (f_offset16) = f_offset16;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_sthbe", "f_dstbe 0x%x", 'x', f_dstbe, "f_imm32 0x%x", 'x', f_imm32, "f_offset16 0x%x", 'x', f_offset16, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_stwbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_stbbe.f
    INT f_imm32;
    HI f_offset16;
    UINT f_dstbe;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstbe) = f_dstbe;
  FLD (f_imm32) = f_imm32;
  FLD (f_offset16) = f_offset16;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stwbe", "f_dstbe 0x%x", 'x', f_dstbe, "f_imm32 0x%x", 'x', f_imm32, "f_offset16 0x%x", 'x', f_offset16, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_stdwbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_stbbe.f
    INT f_imm32;
    HI f_offset16;
    UINT f_dstbe;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstbe) = f_dstbe;
  FLD (f_imm32) = f_imm32;
  FLD (f_offset16) = f_offset16;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stdwbe", "f_dstbe 0x%x", 'x', f_dstbe, "f_imm32 0x%x", 'x', f_imm32, "f_offset16 0x%x", 'x', f_offset16, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_jeqibe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_stbbe.f
    INT f_imm32;
    HI f_offset16;
    UINT f_dstbe;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_offset16) = f_offset16;
  FLD (f_dstbe) = f_dstbe;
  FLD (f_imm32) = f_imm32;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_jeqibe", "f_offset16 0x%x", 'x', f_offset16, "f_dstbe 0x%x", 'x', f_dstbe, "f_imm32 0x%x", 'x', f_imm32, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_jeqrbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
    HI f_offset16;
    UINT f_dstbe;
    UINT f_srcbe;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_srcbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_offset16) = f_offset16;
  FLD (f_dstbe) = f_dstbe;
  FLD (f_srcbe) = f_srcbe;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_jeqrbe", "f_offset16 0x%x", 'x', f_offset16, "f_dstbe 0x%x", 'x', f_dstbe, "f_srcbe 0x%x", 'x', f_srcbe, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_callbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldindwbe.f
    INT f_imm32;
    UINT f_srcbe;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_srcbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_imm32) = f_imm32;
  FLD (f_srcbe) = f_srcbe;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_callbe", "f_imm32 0x%x", 'x', f_imm32, "f_srcbe 0x%x", 'x', f_srcbe, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ja:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_stbbe.f
    HI f_offset16;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_offset16) = f_offset16;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ja", "f_offset16 0x%x", 'x', f_offset16, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_exit:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_empty.f


  /* Record the fields for the semantic handler.  */
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_exit", (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_xadddwbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
    HI f_offset16;
    UINT f_dstbe;
    UINT f_srcbe;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_srcbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstbe) = f_dstbe;
  FLD (f_offset16) = f_offset16;
  FLD (f_srcbe) = f_srcbe;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_xadddwbe", "f_dstbe 0x%x", 'x', f_dstbe, "f_offset16 0x%x", 'x', f_offset16, "f_srcbe 0x%x", 'x', f_srcbe, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_xaddwbe:
  {
    const IDESC *idesc = &bpfbf_ebpfbe_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
    HI f_offset16;
    UINT f_dstbe;
    UINT f_srcbe;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_dstbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_srcbe = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstbe) = f_dstbe;
  FLD (f_offset16) = f_offset16;
  FLD (f_srcbe) = f_srcbe;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_xaddwbe", "f_dstbe 0x%x", 'x', f_dstbe, "f_offset16 0x%x", 'x', f_offset16, "f_srcbe 0x%x", 'x', f_srcbe, (char *) 0));

#undef FLD
    return idesc;
  }

}

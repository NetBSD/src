/* Simulator instruction decoder for bpfbf_ebpfle.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright (C) 1996-2020 Free Software Foundation, Inc.

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

/* The instruction descriptor array.
   This is computed at runtime.  Space for it is not malloc'd to save a
   teensy bit of cpu in the decoder.  Moving it to malloc space is trivial
   but won't be done until necessary (we don't currently support the runtime
   addition of instructions nor an SMP machine with different cpus).  */
static IDESC bpfbf_ebpfle_insn_data[BPFBF_EBPFLE_INSN__MAX];

/* Commas between elements are contained in the macros.
   Some of these are conditionally compiled out.  */

static const struct insn_sem bpfbf_ebpfle_insn_sem[] =
{
  { VIRTUAL_INSN_X_INVALID, BPFBF_EBPFLE_INSN_X_INVALID, BPFBF_EBPFLE_SFMT_EMPTY },
  { VIRTUAL_INSN_X_AFTER, BPFBF_EBPFLE_INSN_X_AFTER, BPFBF_EBPFLE_SFMT_EMPTY },
  { VIRTUAL_INSN_X_BEFORE, BPFBF_EBPFLE_INSN_X_BEFORE, BPFBF_EBPFLE_SFMT_EMPTY },
  { VIRTUAL_INSN_X_CTI_CHAIN, BPFBF_EBPFLE_INSN_X_CTI_CHAIN, BPFBF_EBPFLE_SFMT_EMPTY },
  { VIRTUAL_INSN_X_CHAIN, BPFBF_EBPFLE_INSN_X_CHAIN, BPFBF_EBPFLE_SFMT_EMPTY },
  { VIRTUAL_INSN_X_BEGIN, BPFBF_EBPFLE_INSN_X_BEGIN, BPFBF_EBPFLE_SFMT_EMPTY },
  { BPF_INSN_ADDILE, BPFBF_EBPFLE_INSN_ADDILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_ADDRLE, BPFBF_EBPFLE_INSN_ADDRLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_ADD32ILE, BPFBF_EBPFLE_INSN_ADD32ILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_ADD32RLE, BPFBF_EBPFLE_INSN_ADD32RLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_SUBILE, BPFBF_EBPFLE_INSN_SUBILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_SUBRLE, BPFBF_EBPFLE_INSN_SUBRLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_SUB32ILE, BPFBF_EBPFLE_INSN_SUB32ILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_SUB32RLE, BPFBF_EBPFLE_INSN_SUB32RLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_MULILE, BPFBF_EBPFLE_INSN_MULILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_MULRLE, BPFBF_EBPFLE_INSN_MULRLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_MUL32ILE, BPFBF_EBPFLE_INSN_MUL32ILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_MUL32RLE, BPFBF_EBPFLE_INSN_MUL32RLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_DIVILE, BPFBF_EBPFLE_INSN_DIVILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_DIVRLE, BPFBF_EBPFLE_INSN_DIVRLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_DIV32ILE, BPFBF_EBPFLE_INSN_DIV32ILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_DIV32RLE, BPFBF_EBPFLE_INSN_DIV32RLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_ORILE, BPFBF_EBPFLE_INSN_ORILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_ORRLE, BPFBF_EBPFLE_INSN_ORRLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_OR32ILE, BPFBF_EBPFLE_INSN_OR32ILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_OR32RLE, BPFBF_EBPFLE_INSN_OR32RLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_ANDILE, BPFBF_EBPFLE_INSN_ANDILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_ANDRLE, BPFBF_EBPFLE_INSN_ANDRLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_AND32ILE, BPFBF_EBPFLE_INSN_AND32ILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_AND32RLE, BPFBF_EBPFLE_INSN_AND32RLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_LSHILE, BPFBF_EBPFLE_INSN_LSHILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_LSHRLE, BPFBF_EBPFLE_INSN_LSHRLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_LSH32ILE, BPFBF_EBPFLE_INSN_LSH32ILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_LSH32RLE, BPFBF_EBPFLE_INSN_LSH32RLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_RSHILE, BPFBF_EBPFLE_INSN_RSHILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_RSHRLE, BPFBF_EBPFLE_INSN_RSHRLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_RSH32ILE, BPFBF_EBPFLE_INSN_RSH32ILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_RSH32RLE, BPFBF_EBPFLE_INSN_RSH32RLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_MODILE, BPFBF_EBPFLE_INSN_MODILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_MODRLE, BPFBF_EBPFLE_INSN_MODRLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_MOD32ILE, BPFBF_EBPFLE_INSN_MOD32ILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_MOD32RLE, BPFBF_EBPFLE_INSN_MOD32RLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_XORILE, BPFBF_EBPFLE_INSN_XORILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_XORRLE, BPFBF_EBPFLE_INSN_XORRLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_XOR32ILE, BPFBF_EBPFLE_INSN_XOR32ILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_XOR32RLE, BPFBF_EBPFLE_INSN_XOR32RLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_ARSHILE, BPFBF_EBPFLE_INSN_ARSHILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_ARSHRLE, BPFBF_EBPFLE_INSN_ARSHRLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_ARSH32ILE, BPFBF_EBPFLE_INSN_ARSH32ILE, BPFBF_EBPFLE_SFMT_ADDILE },
  { BPF_INSN_ARSH32RLE, BPFBF_EBPFLE_INSN_ARSH32RLE, BPFBF_EBPFLE_SFMT_ADDRLE },
  { BPF_INSN_NEGLE, BPFBF_EBPFLE_INSN_NEGLE, BPFBF_EBPFLE_SFMT_NEGLE },
  { BPF_INSN_NEG32LE, BPFBF_EBPFLE_INSN_NEG32LE, BPFBF_EBPFLE_SFMT_NEGLE },
  { BPF_INSN_MOVILE, BPFBF_EBPFLE_INSN_MOVILE, BPFBF_EBPFLE_SFMT_MOVILE },
  { BPF_INSN_MOVRLE, BPFBF_EBPFLE_INSN_MOVRLE, BPFBF_EBPFLE_SFMT_MOVRLE },
  { BPF_INSN_MOV32ILE, BPFBF_EBPFLE_INSN_MOV32ILE, BPFBF_EBPFLE_SFMT_MOVILE },
  { BPF_INSN_MOV32RLE, BPFBF_EBPFLE_INSN_MOV32RLE, BPFBF_EBPFLE_SFMT_MOVRLE },
  { BPF_INSN_ENDLELE, BPFBF_EBPFLE_INSN_ENDLELE, BPFBF_EBPFLE_SFMT_ENDLELE },
  { BPF_INSN_ENDBELE, BPFBF_EBPFLE_INSN_ENDBELE, BPFBF_EBPFLE_SFMT_ENDLELE },
  { BPF_INSN_LDDWLE, BPFBF_EBPFLE_INSN_LDDWLE, BPFBF_EBPFLE_SFMT_LDDWLE },
  { BPF_INSN_LDABSW, BPFBF_EBPFLE_INSN_LDABSW, BPFBF_EBPFLE_SFMT_LDABSW },
  { BPF_INSN_LDABSH, BPFBF_EBPFLE_INSN_LDABSH, BPFBF_EBPFLE_SFMT_LDABSH },
  { BPF_INSN_LDABSB, BPFBF_EBPFLE_INSN_LDABSB, BPFBF_EBPFLE_SFMT_LDABSB },
  { BPF_INSN_LDABSDW, BPFBF_EBPFLE_INSN_LDABSDW, BPFBF_EBPFLE_SFMT_LDABSDW },
  { BPF_INSN_LDINDWLE, BPFBF_EBPFLE_INSN_LDINDWLE, BPFBF_EBPFLE_SFMT_LDINDWLE },
  { BPF_INSN_LDINDHLE, BPFBF_EBPFLE_INSN_LDINDHLE, BPFBF_EBPFLE_SFMT_LDINDHLE },
  { BPF_INSN_LDINDBLE, BPFBF_EBPFLE_INSN_LDINDBLE, BPFBF_EBPFLE_SFMT_LDINDBLE },
  { BPF_INSN_LDINDDWLE, BPFBF_EBPFLE_INSN_LDINDDWLE, BPFBF_EBPFLE_SFMT_LDINDDWLE },
  { BPF_INSN_LDXWLE, BPFBF_EBPFLE_INSN_LDXWLE, BPFBF_EBPFLE_SFMT_LDXWLE },
  { BPF_INSN_LDXHLE, BPFBF_EBPFLE_INSN_LDXHLE, BPFBF_EBPFLE_SFMT_LDXHLE },
  { BPF_INSN_LDXBLE, BPFBF_EBPFLE_INSN_LDXBLE, BPFBF_EBPFLE_SFMT_LDXBLE },
  { BPF_INSN_LDXDWLE, BPFBF_EBPFLE_INSN_LDXDWLE, BPFBF_EBPFLE_SFMT_LDXDWLE },
  { BPF_INSN_STXWLE, BPFBF_EBPFLE_INSN_STXWLE, BPFBF_EBPFLE_SFMT_STXWLE },
  { BPF_INSN_STXHLE, BPFBF_EBPFLE_INSN_STXHLE, BPFBF_EBPFLE_SFMT_STXHLE },
  { BPF_INSN_STXBLE, BPFBF_EBPFLE_INSN_STXBLE, BPFBF_EBPFLE_SFMT_STXBLE },
  { BPF_INSN_STXDWLE, BPFBF_EBPFLE_INSN_STXDWLE, BPFBF_EBPFLE_SFMT_STXDWLE },
  { BPF_INSN_STBLE, BPFBF_EBPFLE_INSN_STBLE, BPFBF_EBPFLE_SFMT_STBLE },
  { BPF_INSN_STHLE, BPFBF_EBPFLE_INSN_STHLE, BPFBF_EBPFLE_SFMT_STHLE },
  { BPF_INSN_STWLE, BPFBF_EBPFLE_INSN_STWLE, BPFBF_EBPFLE_SFMT_STWLE },
  { BPF_INSN_STDWLE, BPFBF_EBPFLE_INSN_STDWLE, BPFBF_EBPFLE_SFMT_STDWLE },
  { BPF_INSN_JEQILE, BPFBF_EBPFLE_INSN_JEQILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JEQRLE, BPFBF_EBPFLE_INSN_JEQRLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_JEQ32ILE, BPFBF_EBPFLE_INSN_JEQ32ILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JEQ32RLE, BPFBF_EBPFLE_INSN_JEQ32RLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_JGTILE, BPFBF_EBPFLE_INSN_JGTILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JGTRLE, BPFBF_EBPFLE_INSN_JGTRLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_JGT32ILE, BPFBF_EBPFLE_INSN_JGT32ILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JGT32RLE, BPFBF_EBPFLE_INSN_JGT32RLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_JGEILE, BPFBF_EBPFLE_INSN_JGEILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JGERLE, BPFBF_EBPFLE_INSN_JGERLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_JGE32ILE, BPFBF_EBPFLE_INSN_JGE32ILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JGE32RLE, BPFBF_EBPFLE_INSN_JGE32RLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_JLTILE, BPFBF_EBPFLE_INSN_JLTILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JLTRLE, BPFBF_EBPFLE_INSN_JLTRLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_JLT32ILE, BPFBF_EBPFLE_INSN_JLT32ILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JLT32RLE, BPFBF_EBPFLE_INSN_JLT32RLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_JLEILE, BPFBF_EBPFLE_INSN_JLEILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JLERLE, BPFBF_EBPFLE_INSN_JLERLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_JLE32ILE, BPFBF_EBPFLE_INSN_JLE32ILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JLE32RLE, BPFBF_EBPFLE_INSN_JLE32RLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_JSETILE, BPFBF_EBPFLE_INSN_JSETILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JSETRLE, BPFBF_EBPFLE_INSN_JSETRLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_JSET32ILE, BPFBF_EBPFLE_INSN_JSET32ILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JSET32RLE, BPFBF_EBPFLE_INSN_JSET32RLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_JNEILE, BPFBF_EBPFLE_INSN_JNEILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JNERLE, BPFBF_EBPFLE_INSN_JNERLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_JNE32ILE, BPFBF_EBPFLE_INSN_JNE32ILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JNE32RLE, BPFBF_EBPFLE_INSN_JNE32RLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_JSGTILE, BPFBF_EBPFLE_INSN_JSGTILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JSGTRLE, BPFBF_EBPFLE_INSN_JSGTRLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_JSGT32ILE, BPFBF_EBPFLE_INSN_JSGT32ILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JSGT32RLE, BPFBF_EBPFLE_INSN_JSGT32RLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_JSGEILE, BPFBF_EBPFLE_INSN_JSGEILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JSGERLE, BPFBF_EBPFLE_INSN_JSGERLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_JSGE32ILE, BPFBF_EBPFLE_INSN_JSGE32ILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JSGE32RLE, BPFBF_EBPFLE_INSN_JSGE32RLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_JSLTILE, BPFBF_EBPFLE_INSN_JSLTILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JSLTRLE, BPFBF_EBPFLE_INSN_JSLTRLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_JSLT32ILE, BPFBF_EBPFLE_INSN_JSLT32ILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JSLT32RLE, BPFBF_EBPFLE_INSN_JSLT32RLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_JSLEILE, BPFBF_EBPFLE_INSN_JSLEILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JSLERLE, BPFBF_EBPFLE_INSN_JSLERLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_JSLE32ILE, BPFBF_EBPFLE_INSN_JSLE32ILE, BPFBF_EBPFLE_SFMT_JEQILE },
  { BPF_INSN_JSLE32RLE, BPFBF_EBPFLE_INSN_JSLE32RLE, BPFBF_EBPFLE_SFMT_JEQRLE },
  { BPF_INSN_CALLLE, BPFBF_EBPFLE_INSN_CALLLE, BPFBF_EBPFLE_SFMT_CALLLE },
  { BPF_INSN_JA, BPFBF_EBPFLE_INSN_JA, BPFBF_EBPFLE_SFMT_JA },
  { BPF_INSN_EXIT, BPFBF_EBPFLE_INSN_EXIT, BPFBF_EBPFLE_SFMT_EXIT },
  { BPF_INSN_XADDDWLE, BPFBF_EBPFLE_INSN_XADDDWLE, BPFBF_EBPFLE_SFMT_XADDDWLE },
  { BPF_INSN_XADDWLE, BPFBF_EBPFLE_INSN_XADDWLE, BPFBF_EBPFLE_SFMT_XADDWLE },
  { BPF_INSN_BRKPT, BPFBF_EBPFLE_INSN_BRKPT, BPFBF_EBPFLE_SFMT_EXIT },
};

static const struct insn_sem bpfbf_ebpfle_insn_sem_invalid =
{
  VIRTUAL_INSN_X_INVALID, BPFBF_EBPFLE_INSN_X_INVALID, BPFBF_EBPFLE_SFMT_EMPTY
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
bpfbf_ebpfle_init_idesc_table (SIM_CPU *cpu)
{
  IDESC *id,*tabend;
  const struct insn_sem *t,*tend;
  int tabsize = BPFBF_EBPFLE_INSN__MAX;
  IDESC *table = bpfbf_ebpfle_insn_data;

  memset (table, 0, tabsize * sizeof (IDESC));

  /* First set all entries to the `invalid insn'.  */
  t = & bpfbf_ebpfle_insn_sem_invalid;
  for (id = table, tabend = table + tabsize; id < tabend; ++id)
    init_idesc (cpu, id, t);

  /* Now fill in the values for the chosen cpu.  */
  for (t = bpfbf_ebpfle_insn_sem, tend = t + sizeof (bpfbf_ebpfle_insn_sem) / sizeof (*t);
       t != tend; ++t)
    {
      init_idesc (cpu, & table[t->index], t);
    }

  /* Link the IDESC table into the cpu.  */
  CPU_IDESC (cpu) = table;
}

/* Given an instruction, return a pointer to its IDESC entry.  */

const IDESC *
bpfbf_ebpfle_decode (SIM_CPU *current_cpu, IADDR pc,
              CGEN_INSN_WORD base_insn,
              ARGBUF *abuf)
{
  /* Result of decoder.  */
  BPFBF_EBPFLE_INSN_TYPE itype;

  {
    CGEN_INSN_WORD insn = base_insn;

    {
      unsigned int val = (((insn >> 0) & (255 << 0)));
      switch (val)
      {
      case 4 : itype = BPFBF_EBPFLE_INSN_ADD32ILE; goto extract_sfmt_addile;
      case 5 : itype = BPFBF_EBPFLE_INSN_JA; goto extract_sfmt_ja;
      case 7 : itype = BPFBF_EBPFLE_INSN_ADDILE; goto extract_sfmt_addile;
      case 12 : itype = BPFBF_EBPFLE_INSN_ADD32RLE; goto extract_sfmt_addrle;
      case 15 : itype = BPFBF_EBPFLE_INSN_ADDRLE; goto extract_sfmt_addrle;
      case 20 : itype = BPFBF_EBPFLE_INSN_SUB32ILE; goto extract_sfmt_addile;
      case 21 : itype = BPFBF_EBPFLE_INSN_JEQILE; goto extract_sfmt_jeqile;
      case 22 : itype = BPFBF_EBPFLE_INSN_JEQ32ILE; goto extract_sfmt_jeqile;
      case 23 : itype = BPFBF_EBPFLE_INSN_SUBILE; goto extract_sfmt_addile;
      case 24 : itype = BPFBF_EBPFLE_INSN_LDDWLE; goto extract_sfmt_lddwle;
      case 28 : itype = BPFBF_EBPFLE_INSN_SUB32RLE; goto extract_sfmt_addrle;
      case 29 : itype = BPFBF_EBPFLE_INSN_JEQRLE; goto extract_sfmt_jeqrle;
      case 30 : itype = BPFBF_EBPFLE_INSN_JEQ32RLE; goto extract_sfmt_jeqrle;
      case 31 : itype = BPFBF_EBPFLE_INSN_SUBRLE; goto extract_sfmt_addrle;
      case 32 : itype = BPFBF_EBPFLE_INSN_LDABSW; goto extract_sfmt_ldabsw;
      case 36 : itype = BPFBF_EBPFLE_INSN_MUL32ILE; goto extract_sfmt_addile;
      case 37 : itype = BPFBF_EBPFLE_INSN_JGTILE; goto extract_sfmt_jeqile;
      case 38 : itype = BPFBF_EBPFLE_INSN_JGT32ILE; goto extract_sfmt_jeqile;
      case 39 : itype = BPFBF_EBPFLE_INSN_MULILE; goto extract_sfmt_addile;
      case 40 : itype = BPFBF_EBPFLE_INSN_LDABSH; goto extract_sfmt_ldabsh;
      case 44 : itype = BPFBF_EBPFLE_INSN_MUL32RLE; goto extract_sfmt_addrle;
      case 45 : itype = BPFBF_EBPFLE_INSN_JGTRLE; goto extract_sfmt_jeqrle;
      case 46 : itype = BPFBF_EBPFLE_INSN_JGT32RLE; goto extract_sfmt_jeqrle;
      case 47 : itype = BPFBF_EBPFLE_INSN_MULRLE; goto extract_sfmt_addrle;
      case 48 : itype = BPFBF_EBPFLE_INSN_LDABSB; goto extract_sfmt_ldabsb;
      case 52 : itype = BPFBF_EBPFLE_INSN_DIV32ILE; goto extract_sfmt_addile;
      case 53 : itype = BPFBF_EBPFLE_INSN_JGEILE; goto extract_sfmt_jeqile;
      case 54 : itype = BPFBF_EBPFLE_INSN_JGE32ILE; goto extract_sfmt_jeqile;
      case 55 : itype = BPFBF_EBPFLE_INSN_DIVILE; goto extract_sfmt_addile;
      case 56 : itype = BPFBF_EBPFLE_INSN_LDABSDW; goto extract_sfmt_ldabsdw;
      case 60 : itype = BPFBF_EBPFLE_INSN_DIV32RLE; goto extract_sfmt_addrle;
      case 61 : itype = BPFBF_EBPFLE_INSN_JGERLE; goto extract_sfmt_jeqrle;
      case 62 : itype = BPFBF_EBPFLE_INSN_JGE32RLE; goto extract_sfmt_jeqrle;
      case 63 : itype = BPFBF_EBPFLE_INSN_DIVRLE; goto extract_sfmt_addrle;
      case 64 : itype = BPFBF_EBPFLE_INSN_LDINDWLE; goto extract_sfmt_ldindwle;
      case 68 : itype = BPFBF_EBPFLE_INSN_OR32ILE; goto extract_sfmt_addile;
      case 69 : itype = BPFBF_EBPFLE_INSN_JSETILE; goto extract_sfmt_jeqile;
      case 70 : itype = BPFBF_EBPFLE_INSN_JSET32ILE; goto extract_sfmt_jeqile;
      case 71 : itype = BPFBF_EBPFLE_INSN_ORILE; goto extract_sfmt_addile;
      case 72 : itype = BPFBF_EBPFLE_INSN_LDINDHLE; goto extract_sfmt_ldindhle;
      case 76 : itype = BPFBF_EBPFLE_INSN_OR32RLE; goto extract_sfmt_addrle;
      case 77 : itype = BPFBF_EBPFLE_INSN_JSETRLE; goto extract_sfmt_jeqrle;
      case 78 : itype = BPFBF_EBPFLE_INSN_JSET32RLE; goto extract_sfmt_jeqrle;
      case 79 : itype = BPFBF_EBPFLE_INSN_ORRLE; goto extract_sfmt_addrle;
      case 80 : itype = BPFBF_EBPFLE_INSN_LDINDBLE; goto extract_sfmt_ldindble;
      case 84 : itype = BPFBF_EBPFLE_INSN_AND32ILE; goto extract_sfmt_addile;
      case 85 : itype = BPFBF_EBPFLE_INSN_JNEILE; goto extract_sfmt_jeqile;
      case 86 : itype = BPFBF_EBPFLE_INSN_JNE32ILE; goto extract_sfmt_jeqile;
      case 87 : itype = BPFBF_EBPFLE_INSN_ANDILE; goto extract_sfmt_addile;
      case 88 : itype = BPFBF_EBPFLE_INSN_LDINDDWLE; goto extract_sfmt_ldinddwle;
      case 92 : itype = BPFBF_EBPFLE_INSN_AND32RLE; goto extract_sfmt_addrle;
      case 93 : itype = BPFBF_EBPFLE_INSN_JNERLE; goto extract_sfmt_jeqrle;
      case 94 : itype = BPFBF_EBPFLE_INSN_JNE32RLE; goto extract_sfmt_jeqrle;
      case 95 : itype = BPFBF_EBPFLE_INSN_ANDRLE; goto extract_sfmt_addrle;
      case 97 : itype = BPFBF_EBPFLE_INSN_LDXWLE; goto extract_sfmt_ldxwle;
      case 98 : itype = BPFBF_EBPFLE_INSN_STWLE; goto extract_sfmt_stwle;
      case 99 : itype = BPFBF_EBPFLE_INSN_STXWLE; goto extract_sfmt_stxwle;
      case 100 : itype = BPFBF_EBPFLE_INSN_LSH32ILE; goto extract_sfmt_addile;
      case 101 : itype = BPFBF_EBPFLE_INSN_JSGTILE; goto extract_sfmt_jeqile;
      case 102 : itype = BPFBF_EBPFLE_INSN_JSGT32ILE; goto extract_sfmt_jeqile;
      case 103 : itype = BPFBF_EBPFLE_INSN_LSHILE; goto extract_sfmt_addile;
      case 105 : itype = BPFBF_EBPFLE_INSN_LDXHLE; goto extract_sfmt_ldxhle;
      case 106 : itype = BPFBF_EBPFLE_INSN_STHLE; goto extract_sfmt_sthle;
      case 107 : itype = BPFBF_EBPFLE_INSN_STXHLE; goto extract_sfmt_stxhle;
      case 108 : itype = BPFBF_EBPFLE_INSN_LSH32RLE; goto extract_sfmt_addrle;
      case 109 : itype = BPFBF_EBPFLE_INSN_JSGTRLE; goto extract_sfmt_jeqrle;
      case 110 : itype = BPFBF_EBPFLE_INSN_JSGT32RLE; goto extract_sfmt_jeqrle;
      case 111 : itype = BPFBF_EBPFLE_INSN_LSHRLE; goto extract_sfmt_addrle;
      case 113 : itype = BPFBF_EBPFLE_INSN_LDXBLE; goto extract_sfmt_ldxble;
      case 114 : itype = BPFBF_EBPFLE_INSN_STBLE; goto extract_sfmt_stble;
      case 115 : itype = BPFBF_EBPFLE_INSN_STXBLE; goto extract_sfmt_stxble;
      case 116 : itype = BPFBF_EBPFLE_INSN_RSH32ILE; goto extract_sfmt_addile;
      case 117 : itype = BPFBF_EBPFLE_INSN_JSGEILE; goto extract_sfmt_jeqile;
      case 118 : itype = BPFBF_EBPFLE_INSN_JSGE32ILE; goto extract_sfmt_jeqile;
      case 119 : itype = BPFBF_EBPFLE_INSN_RSHILE; goto extract_sfmt_addile;
      case 121 : itype = BPFBF_EBPFLE_INSN_LDXDWLE; goto extract_sfmt_ldxdwle;
      case 122 : itype = BPFBF_EBPFLE_INSN_STDWLE; goto extract_sfmt_stdwle;
      case 123 : itype = BPFBF_EBPFLE_INSN_STXDWLE; goto extract_sfmt_stxdwle;
      case 124 : itype = BPFBF_EBPFLE_INSN_RSH32RLE; goto extract_sfmt_addrle;
      case 125 : itype = BPFBF_EBPFLE_INSN_JSGERLE; goto extract_sfmt_jeqrle;
      case 126 : itype = BPFBF_EBPFLE_INSN_JSGE32RLE; goto extract_sfmt_jeqrle;
      case 127 : itype = BPFBF_EBPFLE_INSN_RSHRLE; goto extract_sfmt_addrle;
      case 132 : itype = BPFBF_EBPFLE_INSN_NEG32LE; goto extract_sfmt_negle;
      case 133 : itype = BPFBF_EBPFLE_INSN_CALLLE; goto extract_sfmt_callle;
      case 135 : itype = BPFBF_EBPFLE_INSN_NEGLE; goto extract_sfmt_negle;
      case 140 : itype = BPFBF_EBPFLE_INSN_BRKPT; goto extract_sfmt_exit;
      case 148 : itype = BPFBF_EBPFLE_INSN_MOD32ILE; goto extract_sfmt_addile;
      case 149 : itype = BPFBF_EBPFLE_INSN_EXIT; goto extract_sfmt_exit;
      case 151 : itype = BPFBF_EBPFLE_INSN_MODILE; goto extract_sfmt_addile;
      case 156 : itype = BPFBF_EBPFLE_INSN_MOD32RLE; goto extract_sfmt_addrle;
      case 159 : itype = BPFBF_EBPFLE_INSN_MODRLE; goto extract_sfmt_addrle;
      case 164 : itype = BPFBF_EBPFLE_INSN_XOR32ILE; goto extract_sfmt_addile;
      case 165 : itype = BPFBF_EBPFLE_INSN_JLTILE; goto extract_sfmt_jeqile;
      case 166 : itype = BPFBF_EBPFLE_INSN_JLT32ILE; goto extract_sfmt_jeqile;
      case 167 : itype = BPFBF_EBPFLE_INSN_XORILE; goto extract_sfmt_addile;
      case 172 : itype = BPFBF_EBPFLE_INSN_XOR32RLE; goto extract_sfmt_addrle;
      case 173 : itype = BPFBF_EBPFLE_INSN_JLTRLE; goto extract_sfmt_jeqrle;
      case 174 : itype = BPFBF_EBPFLE_INSN_JLT32RLE; goto extract_sfmt_jeqrle;
      case 175 : itype = BPFBF_EBPFLE_INSN_XORRLE; goto extract_sfmt_addrle;
      case 180 : itype = BPFBF_EBPFLE_INSN_MOV32ILE; goto extract_sfmt_movile;
      case 181 : itype = BPFBF_EBPFLE_INSN_JLEILE; goto extract_sfmt_jeqile;
      case 182 : itype = BPFBF_EBPFLE_INSN_JLE32ILE; goto extract_sfmt_jeqile;
      case 183 : itype = BPFBF_EBPFLE_INSN_MOVILE; goto extract_sfmt_movile;
      case 188 : itype = BPFBF_EBPFLE_INSN_MOV32RLE; goto extract_sfmt_movrle;
      case 189 : itype = BPFBF_EBPFLE_INSN_JLERLE; goto extract_sfmt_jeqrle;
      case 190 : itype = BPFBF_EBPFLE_INSN_JLE32RLE; goto extract_sfmt_jeqrle;
      case 191 : itype = BPFBF_EBPFLE_INSN_MOVRLE; goto extract_sfmt_movrle;
      case 195 : itype = BPFBF_EBPFLE_INSN_XADDWLE; goto extract_sfmt_xaddwle;
      case 196 : itype = BPFBF_EBPFLE_INSN_ARSH32ILE; goto extract_sfmt_addile;
      case 197 : itype = BPFBF_EBPFLE_INSN_JSLTILE; goto extract_sfmt_jeqile;
      case 198 : itype = BPFBF_EBPFLE_INSN_JSLT32ILE; goto extract_sfmt_jeqile;
      case 199 : itype = BPFBF_EBPFLE_INSN_ARSHILE; goto extract_sfmt_addile;
      case 204 : itype = BPFBF_EBPFLE_INSN_ARSH32RLE; goto extract_sfmt_addrle;
      case 205 : itype = BPFBF_EBPFLE_INSN_JSLTRLE; goto extract_sfmt_jeqrle;
      case 206 : itype = BPFBF_EBPFLE_INSN_JSLT32RLE; goto extract_sfmt_jeqrle;
      case 207 : itype = BPFBF_EBPFLE_INSN_ARSHRLE; goto extract_sfmt_addrle;
      case 212 : itype = BPFBF_EBPFLE_INSN_ENDLELE; goto extract_sfmt_endlele;
      case 213 : itype = BPFBF_EBPFLE_INSN_JSLEILE; goto extract_sfmt_jeqile;
      case 214 : itype = BPFBF_EBPFLE_INSN_JSLE32ILE; goto extract_sfmt_jeqile;
      case 219 : itype = BPFBF_EBPFLE_INSN_XADDDWLE; goto extract_sfmt_xadddwle;
      case 220 : itype = BPFBF_EBPFLE_INSN_ENDBELE; goto extract_sfmt_endlele;
      case 221 : itype = BPFBF_EBPFLE_INSN_JSLERLE; goto extract_sfmt_jeqrle;
      case 222 : itype = BPFBF_EBPFLE_INSN_JSLE32RLE; goto extract_sfmt_jeqrle;
      default : itype = BPFBF_EBPFLE_INSN_X_INVALID; goto extract_sfmt_empty;
      }
    }
  }

  /* The instruction has been decoded, now extract the fields.  */

 extract_sfmt_empty:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_empty.f


  /* Record the fields for the semantic handler.  */
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_empty", (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_addile:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_stble.f
    INT f_imm32;
    UINT f_dstle;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstle) = f_dstle;
  FLD (f_imm32) = f_imm32;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addile", "f_dstle 0x%x", 'x', f_dstle, "f_imm32 0x%x", 'x', f_imm32, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_addrle:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwle.f
    UINT f_srcle;
    UINT f_dstle;

    f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstle) = f_dstle;
  FLD (f_srcle) = f_srcle;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addrle", "f_dstle 0x%x", 'x', f_dstle, "f_srcle 0x%x", 'x', f_srcle, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_negle:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_lddwle.f
    UINT f_dstle;

    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstle) = f_dstle;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_negle", "f_dstle 0x%x", 'x', f_dstle, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_movile:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_stble.f
    INT f_imm32;
    UINT f_dstle;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_imm32) = f_imm32;
  FLD (f_dstle) = f_dstle;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_movile", "f_imm32 0x%x", 'x', f_imm32, "f_dstle 0x%x", 'x', f_dstle, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_movrle:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwle.f
    UINT f_srcle;
    UINT f_dstle;

    f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_srcle) = f_srcle;
  FLD (f_dstle) = f_dstle;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_movrle", "f_srcle 0x%x", 'x', f_srcle, "f_dstle 0x%x", 'x', f_dstle, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_endlele:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_stble.f
    INT f_imm32;
    UINT f_dstle;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstle) = f_dstle;
  FLD (f_imm32) = f_imm32;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_endlele", "f_dstle 0x%x", 'x', f_dstle, "f_imm32 0x%x", 'x', f_imm32, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lddwle:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_lddwle.f
    UINT f_imm64_c;
    UINT f_imm64_b;
    UINT f_imm64_a;
    UINT f_dstle;
    DI f_imm64;
    /* Contents of trailing part of insn.  */
    UINT word_1;
    UINT word_2;

  word_1 = GETIMEMUSI (current_cpu, pc + 8);
  word_2 = GETIMEMUSI (current_cpu, pc + 12);
    f_imm64_c = (0|(EXTRACT_LSB0_UINT (word_2, 32, 31, 32) << 0));
    f_imm64_b = (0|(EXTRACT_LSB0_UINT (word_1, 32, 31, 32) << 0));
    f_imm64_a = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));
{
  f_imm64 = ((((((UDI) (UINT) (f_imm64_c))) << (32))) | (((UDI) (UINT) (f_imm64_a))));
}

  /* Record the fields for the semantic handler.  */
  FLD (f_imm64) = f_imm64;
  FLD (f_dstle) = f_dstle;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lddwle", "f_imm64 0x%x", 'x', f_imm64, "f_dstle 0x%x", 'x', f_dstle, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldabsw:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldindwle.f
    INT f_imm32;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_imm32) = f_imm32;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldabsw", "f_imm32 0x%x", 'x', f_imm32, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldabsh:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldindwle.f
    INT f_imm32;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_imm32) = f_imm32;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldabsh", "f_imm32 0x%x", 'x', f_imm32, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldabsb:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldindwle.f
    INT f_imm32;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_imm32) = f_imm32;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldabsb", "f_imm32 0x%x", 'x', f_imm32, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldabsdw:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldindwle.f
    INT f_imm32;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_imm32) = f_imm32;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldabsdw", "f_imm32 0x%x", 'x', f_imm32, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldindwle:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldindwle.f
    INT f_imm32;
    UINT f_srcle;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_imm32) = f_imm32;
  FLD (f_srcle) = f_srcle;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldindwle", "f_imm32 0x%x", 'x', f_imm32, "f_srcle 0x%x", 'x', f_srcle, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldindhle:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldindwle.f
    INT f_imm32;
    UINT f_srcle;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_imm32) = f_imm32;
  FLD (f_srcle) = f_srcle;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldindhle", "f_imm32 0x%x", 'x', f_imm32, "f_srcle 0x%x", 'x', f_srcle, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldindble:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldindwle.f
    INT f_imm32;
    UINT f_srcle;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_imm32) = f_imm32;
  FLD (f_srcle) = f_srcle;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldindble", "f_imm32 0x%x", 'x', f_imm32, "f_srcle 0x%x", 'x', f_srcle, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldinddwle:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldindwle.f
    INT f_imm32;
    UINT f_srcle;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_imm32) = f_imm32;
  FLD (f_srcle) = f_srcle;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldinddwle", "f_imm32 0x%x", 'x', f_imm32, "f_srcle 0x%x", 'x', f_srcle, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldxwle:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwle.f
    HI f_offset16;
    UINT f_srcle;
    UINT f_dstle;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_offset16) = f_offset16;
  FLD (f_srcle) = f_srcle;
  FLD (f_dstle) = f_dstle;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldxwle", "f_offset16 0x%x", 'x', f_offset16, "f_srcle 0x%x", 'x', f_srcle, "f_dstle 0x%x", 'x', f_dstle, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldxhle:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwle.f
    HI f_offset16;
    UINT f_srcle;
    UINT f_dstle;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_offset16) = f_offset16;
  FLD (f_srcle) = f_srcle;
  FLD (f_dstle) = f_dstle;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldxhle", "f_offset16 0x%x", 'x', f_offset16, "f_srcle 0x%x", 'x', f_srcle, "f_dstle 0x%x", 'x', f_dstle, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldxble:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwle.f
    HI f_offset16;
    UINT f_srcle;
    UINT f_dstle;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_offset16) = f_offset16;
  FLD (f_srcle) = f_srcle;
  FLD (f_dstle) = f_dstle;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldxble", "f_offset16 0x%x", 'x', f_offset16, "f_srcle 0x%x", 'x', f_srcle, "f_dstle 0x%x", 'x', f_dstle, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ldxdwle:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwle.f
    HI f_offset16;
    UINT f_srcle;
    UINT f_dstle;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_offset16) = f_offset16;
  FLD (f_srcle) = f_srcle;
  FLD (f_dstle) = f_dstle;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldxdwle", "f_offset16 0x%x", 'x', f_offset16, "f_srcle 0x%x", 'x', f_srcle, "f_dstle 0x%x", 'x', f_dstle, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_stxwle:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwle.f
    HI f_offset16;
    UINT f_srcle;
    UINT f_dstle;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstle) = f_dstle;
  FLD (f_offset16) = f_offset16;
  FLD (f_srcle) = f_srcle;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stxwle", "f_dstle 0x%x", 'x', f_dstle, "f_offset16 0x%x", 'x', f_offset16, "f_srcle 0x%x", 'x', f_srcle, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_stxhle:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwle.f
    HI f_offset16;
    UINT f_srcle;
    UINT f_dstle;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstle) = f_dstle;
  FLD (f_offset16) = f_offset16;
  FLD (f_srcle) = f_srcle;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stxhle", "f_dstle 0x%x", 'x', f_dstle, "f_offset16 0x%x", 'x', f_offset16, "f_srcle 0x%x", 'x', f_srcle, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_stxble:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwle.f
    HI f_offset16;
    UINT f_srcle;
    UINT f_dstle;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstle) = f_dstle;
  FLD (f_offset16) = f_offset16;
  FLD (f_srcle) = f_srcle;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stxble", "f_dstle 0x%x", 'x', f_dstle, "f_offset16 0x%x", 'x', f_offset16, "f_srcle 0x%x", 'x', f_srcle, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_stxdwle:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwle.f
    HI f_offset16;
    UINT f_srcle;
    UINT f_dstle;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstle) = f_dstle;
  FLD (f_offset16) = f_offset16;
  FLD (f_srcle) = f_srcle;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stxdwle", "f_dstle 0x%x", 'x', f_dstle, "f_offset16 0x%x", 'x', f_offset16, "f_srcle 0x%x", 'x', f_srcle, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_stble:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_stble.f
    INT f_imm32;
    HI f_offset16;
    UINT f_dstle;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstle) = f_dstle;
  FLD (f_imm32) = f_imm32;
  FLD (f_offset16) = f_offset16;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stble", "f_dstle 0x%x", 'x', f_dstle, "f_imm32 0x%x", 'x', f_imm32, "f_offset16 0x%x", 'x', f_offset16, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_sthle:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_stble.f
    INT f_imm32;
    HI f_offset16;
    UINT f_dstle;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstle) = f_dstle;
  FLD (f_imm32) = f_imm32;
  FLD (f_offset16) = f_offset16;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_sthle", "f_dstle 0x%x", 'x', f_dstle, "f_imm32 0x%x", 'x', f_imm32, "f_offset16 0x%x", 'x', f_offset16, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_stwle:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_stble.f
    INT f_imm32;
    HI f_offset16;
    UINT f_dstle;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstle) = f_dstle;
  FLD (f_imm32) = f_imm32;
  FLD (f_offset16) = f_offset16;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stwle", "f_dstle 0x%x", 'x', f_dstle, "f_imm32 0x%x", 'x', f_imm32, "f_offset16 0x%x", 'x', f_offset16, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_stdwle:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_stble.f
    INT f_imm32;
    HI f_offset16;
    UINT f_dstle;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstle) = f_dstle;
  FLD (f_imm32) = f_imm32;
  FLD (f_offset16) = f_offset16;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stdwle", "f_dstle 0x%x", 'x', f_dstle, "f_imm32 0x%x", 'x', f_imm32, "f_offset16 0x%x", 'x', f_offset16, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_jeqile:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_stble.f
    INT f_imm32;
    HI f_offset16;
    UINT f_dstle;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_offset16) = f_offset16;
  FLD (f_dstle) = f_dstle;
  FLD (f_imm32) = f_imm32;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_jeqile", "f_offset16 0x%x", 'x', f_offset16, "f_dstle 0x%x", 'x', f_dstle, "f_imm32 0x%x", 'x', f_imm32, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_jeqrle:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwle.f
    HI f_offset16;
    UINT f_srcle;
    UINT f_dstle;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_offset16) = f_offset16;
  FLD (f_dstle) = f_dstle;
  FLD (f_srcle) = f_srcle;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_jeqrle", "f_offset16 0x%x", 'x', f_offset16, "f_dstle 0x%x", 'x', f_dstle, "f_srcle 0x%x", 'x', f_srcle, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_callle:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldindwle.f
    INT f_imm32;
    UINT f_srcle;

    f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0));
    f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_imm32) = f_imm32;
  FLD (f_srcle) = f_srcle;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_callle", "f_imm32 0x%x", 'x', f_imm32, "f_srcle 0x%x", 'x', f_srcle, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ja:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_stble.f
    HI f_offset16;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_offset16) = f_offset16;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ja", "f_offset16 0x%x", 'x', f_offset16, (char *) 0));

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
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_empty.f


  /* Record the fields for the semantic handler.  */
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_exit", (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_xadddwle:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwle.f
    HI f_offset16;
    UINT f_srcle;
    UINT f_dstle;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstle) = f_dstle;
  FLD (f_offset16) = f_offset16;
  FLD (f_srcle) = f_srcle;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_xadddwle", "f_dstle 0x%x", 'x', f_dstle, "f_offset16 0x%x", 'x', f_offset16, "f_srcle 0x%x", 'x', f_srcle, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_xaddwle:
  {
    const IDESC *idesc = &bpfbf_ebpfle_insn_data[itype];
    CGEN_INSN_WORD insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldxwle.f
    HI f_offset16;
    UINT f_srcle;
    UINT f_dstle;

    f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0));
    f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0));
    f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstle) = f_dstle;
  FLD (f_offset16) = f_offset16;
  FLD (f_srcle) = f_srcle;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_xaddwle", "f_dstle 0x%x", 'x', f_dstle, "f_offset16 0x%x", 'x', f_offset16, "f_srcle 0x%x", 'x', f_srcle, (char *) 0));

#undef FLD
    return idesc;
  }

}

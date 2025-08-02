/* Linux eBPF support for BFD.
   Copyright (C) 2019-2022 Free Software Foundation, Inc.

   Contributed by Oracle, Inc.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _ELF_BPF_H
#define _ELF_BPF_H

#include "elf/reloc-macros.h"

/* Relocations.  */
START_RELOC_NUMBERS (elf_bpf_reloc_type)
  RELOC_NUMBER (R_BPF_NONE,		0)
  RELOC_NUMBER (R_BPF_INSN_64,		1)
  RELOC_NUMBER (R_BPF_INSN_32,		2)
  RELOC_NUMBER (R_BPF_INSN_16,          3)
  RELOC_NUMBER (R_BPF_INSN_DISP16,	4)
  RELOC_NUMBER (R_BPF_DATA_8_PCREL,	5)
  RELOC_NUMBER (R_BPF_DATA_16_PCREL,	6)
  RELOC_NUMBER (R_BPF_DATA_32_PCREL,	7)
  RELOC_NUMBER (R_BPF_DATA_8,		8)
  RELOC_NUMBER (R_BPF_DATA_16,		9)
  RELOC_NUMBER (R_BPF_INSN_DISP32,	10)
  RELOC_NUMBER (R_BPF_DATA_32,		11)
  RELOC_NUMBER (R_BPF_DATA_64,		12)
  RELOC_NUMBER (R_BPF_DATA_64_PCREL,	13)
END_RELOC_NUMBERS (R_BPF_max)

#endif /* _ELF_BPF_H  */

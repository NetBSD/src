/* Emulation of eBPF helpers.  Interface.
   Copyright (C) 2020-2023 Free Software Foundation, Inc.

   This file is part of GDB, the GNU debugger.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef BPF_HELPERS_H
#define BPF_HELPERS_H

enum bpf_kernel_helper
  {
#define DEF_HELPER(kver, name, fn, types) name,
#include "bpf-helpers.def"
#undef DEF_HELPER
  };

int bpf_trace_printk (SIM_CPU *current_cpu);

VOID bpfbf_breakpoint (SIM_CPU *current_cpu);

#endif /* ! BPF_HELPERS_H */

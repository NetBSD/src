/* eBPF simulator support code header
   Copyright (C) 2020 Free Software Foundation, Inc.

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

#ifndef BPF_SIM_H
#define BPF_SIM_H

void bpfbf_insn_before (sim_cpu* current_cpu, SEM_PC vpc, const IDESC *idesc);
void bpfbf_insn_after (sim_cpu* current_cpu, SEM_PC vpc, const IDESC *idesc);

DI bpfbf_endbe (SIM_CPU *, DI, UINT);
DI bpfbf_endle (SIM_CPU *, DI, UINT);
DI bpfbf_skb_data_offset (SIM_CPU *);
VOID bpfbf_call (SIM_CPU *, INT, UINT);
VOID bpfbf_exit (SIM_CPU *);

#endif /* ! BPF_SIM_H */

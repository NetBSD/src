/* Common target dependent for AArch64 systems.

   Copyright (C) 2018-2020 Free Software Foundation, Inc.

   This file is part of GDB.

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

#ifndef NAT_AARCH64_SVE_LINUX_PTRACE_H
#define NAT_AARCH64_SVE_LINUX_PTRACE_H

#include <signal.h>
#include <sys/utsname.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>

#ifndef SVE_SIG_ZREGS_SIZE
#include "aarch64-sve-linux-sigcontext.h"
#endif

/* Indicates whether a SVE ptrace header is followed by SVE registers or a
   fpsimd structure.  */

#define HAS_SVE_STATE(header) ((header).flags & SVE_PT_REGS_SVE)

/* Read VQ for the given tid using ptrace.  If SVE is not supported then zero
   is returned (on a system that supports SVE, then VQ cannot be zero).  */

uint64_t aarch64_sve_get_vq (int tid);

/* Set VQ in the kernel for the given tid, using either the value VQ or
   reading from the register VG in the register buffer.  */

bool aarch64_sve_set_vq (int tid, uint64_t vq);
bool aarch64_sve_set_vq (int tid, struct reg_buffer_common *reg_buf);

/* Read the current SVE register set using ptrace, allocating space as
   required.  */

extern std::unique_ptr<gdb_byte[]> aarch64_sve_get_sveregs (int tid);

/* Put the registers from linux structure buf into register buffer.  Assumes the
   vector lengths in the register buffer match the size in the kernel.  */

extern void aarch64_sve_regs_copy_to_reg_buf (struct reg_buffer_common *reg_buf,
					      const void *buf);

/* Put the registers from register buffer into linux structure buf.  Assumes the
   vector lengths in the register buffer match the size in the kernel.  */

extern void
aarch64_sve_regs_copy_from_reg_buf (const struct reg_buffer_common *reg_buf,
				    void *buf);

#endif /* NAT_AARCH64_SVE_LINUX_PTRACE_H */

/* Native-dependent code for GNU/Linux on LoongArch processors.

   Copyright (C) 2024-2025 Free Software Foundation, Inc.
   Contributed by Loongson Ltd.

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

#ifndef GDB_NAT_LOONGARCH_LINUX_H
#define GDB_NAT_LOONGARCH_LINUX_H

#include <signal.h>

/* Defines ps_err_e, struct ps_prochandle.  */
#include "gdb_proc_service.h"

/* Called when resuming a thread LWP.
   The hardware debug registers are updated when there is any change.  */

void loongarch_linux_prepare_to_resume (struct lwp_info *lwp);

/* Function to call when a new thread is detected.  */

void loongarch_linux_new_thread (struct lwp_info *lwp);

/* Function to call when a thread is being deleted.  */

void loongarch_linux_delete_thread (struct arch_lwp_info *arch_lwp);

#endif /* GDB_NAT_LOONGARCH_LINUX_H */

/* Copyright (C) 1993 Free Software Foundation, Inc.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#define TARGET_BYTE_ORDER_SELECTABLE

#include "mips/tm-bigmips.h"

#undef DEFAULT_MIPS_TYPE
#define DEFAULT_MIPS_TYPE "r3051"

/* Watchpoint support */

#define TARGET_HAS_HARDWARE_WATCHPOINTS

/* Use these macros for watchpoint insertion/deletion.  */
/* type can be 0: write watch, 1: read watch, 2: access watch (read/write) */

#define target_insert_watchpoint(addr, len, type) \
	remote_mips_set_watchpoint (addr, len, type)
int remote_mips_set_watchpoint PARAMS ((CORE_ADDR addr, int len, int type));

#define target_remove_watchpoint(addr, len, type) \
	remote_mips_remove_watchpoint (addr, len, type)
int remote_mips_remove_watchpoint PARAMS ((CORE_ADDR addr, int len, int type));

/* We need to remove watchpoints when stepping, else we hit them again! */

#define HAVE_NONSTEPPABLE_WATCHPOINT

#define STOPPED_BY_WATCHPOINT(w) remote_mips_stopped_by_watchpoint ()

#define TARGET_CAN_USE_HARDWARE_WATCHPOINT(type, cnt, ot) \
  remote_mips_can_use_hardware_watchpoint(cnt)
int remote_mips_can_use_hardware_watchpoint PARAMS ((int cnt));

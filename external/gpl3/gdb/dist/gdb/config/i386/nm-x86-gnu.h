/* Native-dependent definitions for Intel 386 running the GNU Hurd
   Copyright (C) 1994-2025 Free Software Foundation, Inc.

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

#ifndef GDB_CONFIG_I386_NM_X86_GNU_H
#define GDB_CONFIG_I386_NM_X86_GNU_H

/* Thread flavors used in re-setting the T bit.  */
#define THREAD_STATE_FLAVOR		i386_REGS_SEGS_STATE
#define THREAD_STATE_SIZE		i386_THREAD_STATE_COUNT
#ifdef __x86_64__
#define THREAD_STATE_SET_TRACED(state) \
	((struct i386_thread_state *) (state))->rfl |= 0x100
#define THREAD_STATE_CLEAR_TRACED(state) \
	((((struct i386_thread_state *) (state))->rfl &= ~0x100), 1)
#else
#define THREAD_STATE_SET_TRACED(state) \
  	((struct i386_thread_state *) (state))->efl |= 0x100
#define THREAD_STATE_CLEAR_TRACED(state) \
  	((((struct i386_thread_state *) (state))->efl &= ~0x100), 1)
#endif /* __x86_64__ */

#endif /* GDB_CONFIG_I386_NM_X86_GNU_H */

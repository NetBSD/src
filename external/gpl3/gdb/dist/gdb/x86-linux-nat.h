/* Native-dependent code for GNU/Linux x86 (i386 and x86-64).

   Copyright (C) 1999-2016 Free Software Foundation, Inc.

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

#ifndef X86_LINUX_NAT_H
#define X86_LINUX_NAT_H 1



/* Helper for ps_get_thread_area.  Sets BASE_ADDR to a pointer to
   the thread local storage (or its descriptor) and returns PS_OK
   on success.  Returns PS_ERR on failure.  */

extern ps_err_e x86_linux_get_thread_area (pid_t pid, void *addr,
					   unsigned int *base_addr);


/* Create an x86 GNU/Linux target.  */

extern struct target_ops *x86_linux_create_target (void);

/* Add an x86 GNU/Linux target.  */

extern void x86_linux_add_target (struct target_ops *t);

#endif

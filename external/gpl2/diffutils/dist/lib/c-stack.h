/*	$NetBSD: c-stack.h,v 1.1.1.1 2016/01/13 03:15:30 christos Exp $	*/

/* Stack overflow handling.

   Copyright (C) 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#if ! HAVE_SIGINFO_T
# define siginfo_t void
#endif

int c_stack_action (void (*) (int, siginfo_t *, void *));
void c_stack_die (int, siginfo_t *, void *);

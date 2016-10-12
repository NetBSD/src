/* Portability wrappers for setjmp and longjmp.
   Copyright (C) 1986-2016 Free Software Foundation, Inc.

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

#ifndef GDB_SETJMP_H
#define GDB_SETJMP_H

#include <setjmp.h>

#ifdef HAVE_SIGSETJMP
#define SIGJMP_BUF		sigjmp_buf
#define SIGSETJMP(buf)		sigsetjmp((buf), 1)
#define SIGLONGJMP(buf,val)	siglongjmp((buf), (val))
#else
#define SIGJMP_BUF		jmp_buf
#define SIGSETJMP(buf)		setjmp(buf)
#define SIGLONGJMP(buf,val)	longjmp((buf), (val))
#endif

#endif /* GDB_SETJMP_H */

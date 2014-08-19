/* Common definitions for remote server for GDB.
   Copyright (C) 1993-2014 Free Software Foundation, Inc.

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

#ifndef SERVER_H
#define SERVER_H

#include "config.h"
#include "build-gnulib-gdbserver/config.h"

#ifdef __MINGW32CE__
#include "wincecompat.h"
#endif

#include "libiberty.h"
#include "ansidecl.h"
#include "version.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include <setjmp.h>

/* For gnulib's PATH_MAX.  */
#include "pathmax.h"

#include <string.h>

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
/* On some systems such as MinGW, alloca is declared in malloc.h
   (there is no alloca.h).  */
#if HAVE_MALLOC_H
#include <malloc.h>
#endif

#if !HAVE_DECL_STRERROR
#ifndef strerror
extern char *strerror (int);	/* X3.159-1989  4.11.6.2 */
#endif
#endif

#if !HAVE_DECL_PERROR
#ifndef perror
extern void perror (const char *);
#endif
#endif

#if !HAVE_DECL_VASPRINTF
extern int vasprintf(char **strp, const char *fmt, va_list ap);
#endif
#if !HAVE_DECL_VSNPRINTF
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
#endif

#ifdef IN_PROCESS_AGENT
#  define PROG "ipa"
#else
#  define PROG "gdbserver"
#endif

/* A type used for binary buffers.  */
typedef unsigned char gdb_byte;

#include "ptid.h"
#include "buffer.h"
#include "xml-utils.h"
#include "gdb_locale.h"

/* FIXME: This should probably be autoconf'd for.  It's an integer type at
   least the size of a (void *).  */
typedef long long CORE_ADDR;

typedef long long LONGEST;
typedef unsigned long long ULONGEST;

#include "regcache.h"
#include "gdb/signals.h"
#include "gdb_signals.h"
#include "target.h"
#include "mem-break.h"
#include "gdbthread.h"
#include "inferiors.h"

/* Target-specific functions */

void initialize_low ();

/* Public variables in server.c */

extern ptid_t cont_thread;
extern ptid_t general_thread;

extern int server_waiting;
extern int debug_threads;
extern int debug_hw_points;
extern int pass_signals[];
extern int program_signals[];
extern int program_signals_p;

extern jmp_buf toplevel;

extern int disable_packet_vCont;
extern int disable_packet_Tthread;
extern int disable_packet_qC;
extern int disable_packet_qfThreadInfo;

extern int run_once;
extern int multi_process;
extern int non_stop;

extern int disable_randomization;

#if USE_WIN32API
#include <winsock2.h>
typedef SOCKET gdb_fildes_t;
#else
typedef int gdb_fildes_t;
#endif

#include "event-loop.h"

/* Functions from server.c.  */
extern int handle_serial_event (int err, gdb_client_data client_data);
extern int handle_target_event (int err, gdb_client_data client_data);

#include "remote-utils.h"

#include "common-utils.h"
#include "utils.h"

#include "gdb_assert.h"

/* Maximum number of bytes to read/write at once.  The value here
   is chosen to fill up a packet (the headers account for the 32).  */
#define MAXBUFBYTES(N) (((N)-32)/2)

/* Buffer sizes for transferring memory, registers, etc.   Set to a constant
   value to accomodate multiple register formats.  This value must be at least
   as large as the largest register set supported by gdbserver.  */
#define PBUFSIZ 16384

#endif /* SERVER_H */

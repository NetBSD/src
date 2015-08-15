/* Copyright (C) 2011-2015 Free Software Foundation, Inc.

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

#ifndef COMMON_LINUX_PTRACE_H
#define COMMON_LINUX_PTRACE_H

struct buffer;

#include <sys/ptrace.h>

#ifdef __UCLIBC__
#if !(defined(__UCLIBC_HAS_MMU__) || defined(__ARCH_HAS_MMU__))
/* PTRACE_TEXT_ADDR and friends.  */
#include <asm/ptrace.h>
#define HAS_NOMMU
#endif
#endif

#if !defined(PTRACE_TYPE_ARG3)
#define PTRACE_TYPE_ARG3 void *
#endif

#if !defined(PTRACE_TYPE_ARG4)
#define PTRACE_TYPE_ARG4 void *
#endif

#ifndef PTRACE_GETSIGINFO
# define PTRACE_GETSIGINFO 0x4202
# define PTRACE_SETSIGINFO 0x4203
#endif /* PTRACE_GETSIGINF */

/* If the system headers did not provide the constants, hard-code the normal
   values.  */
#ifndef PTRACE_EVENT_FORK

#define PTRACE_SETOPTIONS	0x4200
#define PTRACE_GETEVENTMSG	0x4201

/* options set using PTRACE_SETOPTIONS */
#define PTRACE_O_TRACESYSGOOD	0x00000001
#define PTRACE_O_TRACEFORK	0x00000002
#define PTRACE_O_TRACEVFORK	0x00000004
#define PTRACE_O_TRACECLONE	0x00000008
#define PTRACE_O_TRACEEXEC	0x00000010
#define PTRACE_O_TRACEVFORKDONE	0x00000020
#define PTRACE_O_TRACEEXIT	0x00000040

/* Wait extended result codes for the above trace options.  */
#define PTRACE_EVENT_FORK	1
#define PTRACE_EVENT_VFORK	2
#define PTRACE_EVENT_CLONE	3
#define PTRACE_EVENT_EXEC	4
#define PTRACE_EVENT_VFORK_DONE	5
#define PTRACE_EVENT_EXIT	6

#endif /* PTRACE_EVENT_FORK */

#ifndef PTRACE_O_EXITKILL
/* Only defined in Linux Kernel 3.8 or later.  */
#define PTRACE_O_EXITKILL	0x00100000
#endif

#if (defined __bfin__ || defined __frv__ || defined __sh__) \
    && !defined PTRACE_GETFDPIC
#define PTRACE_GETFDPIC		31
#define PTRACE_GETFDPIC_EXEC	0
#define PTRACE_GETFDPIC_INTERP	1
#endif

/* We can't always assume that this flag is available, but all systems
   with the ptrace event handlers also have __WALL, so it's safe to use
   in some contexts.  */
#ifndef __WALL
#define __WALL          0x40000000 /* Wait for any child.  */
#endif

extern void linux_ptrace_attach_fail_reason (pid_t pid, struct buffer *buffer);

/* Find all possible reasons we could have failed to attach to PTID
   and return them as a string.  ERR is the error PTRACE_ATTACH failed
   with (an errno).  The result is stored in a static buffer.  This
   string should be copied into a buffer by the client if the string
   will not be immediately used, or if it must persist.  */
extern char *linux_ptrace_attach_fail_reason_string (ptid_t ptid, int err);

extern void linux_ptrace_init_warnings (void);
extern void linux_enable_event_reporting (pid_t pid, int attached);
extern void linux_disable_event_reporting (pid_t pid);
extern int linux_supports_tracefork (void);
extern int linux_supports_traceclone (void);
extern int linux_supports_tracevforkdone (void);
extern int linux_supports_tracesysgood (void);
extern void linux_ptrace_set_additional_flags (int);
extern int linux_ptrace_get_extended_event (int wstat);
extern int linux_is_extended_waitstatus (int wstat);

#endif /* COMMON_LINUX_PTRACE_H */

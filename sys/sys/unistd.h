/*	$NetBSD: unistd.h,v 1.28 2003/05/31 11:50:15 bjh21 Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)unistd.h	8.2 (Berkeley) 1/7/94
 */

#ifndef _SYS_UNISTD_H_
#define	_SYS_UNISTD_H_

#include <sys/featuretest.h>

/* compile-time symbolic constants */
#define	_POSIX_JOB_CONTROL	/* implementation supports job control */

/*
 * According to POSIX 1003.1:
 * "The saved set-user-ID capability allows a program to regain the
 * effective user ID established at the last exec call."
 * However, the setuid/setgid function as specified by POSIX 1003.1 does
 * not allow changing the effective ID from the super-user without also
 * changed the saved ID, so it is impossible to get super-user privileges
 * back later.  Instead we provide this feature independent of the current
 * effective ID through the seteuid/setegid function.  In addition, we do
 * not use the saved ID as specified by POSIX 1003.1 in setuid/setgid,
 * because this would make it impossible for a set-user-ID executable
 * owned by a user other than the super-user to permanently revoke its
 * extra privileges.
 */
#ifdef	_NOT_AVAILABLE
#define	_POSIX_SAVED_IDS	/* saved set-user-ID and set-group-ID */
#endif

#define	_POSIX_VERSION		199009L
#define	_POSIX2_VERSION		199212L

/* execution-time symbolic constants */
				/* chown requires appropriate privileges */
#define	_POSIX_CHOWN_RESTRICTED	1
				/* too-long path components generate errors */
#define	_POSIX_NO_TRUNC		1
				/* may disable terminal special characters */
#define	_POSIX_VDISABLE		((unsigned char)'\377')
				/* file synchronization is available */
#define	_POSIX_FSYNC		1
				/* synchronized I/O is available */
#define	_POSIX_SYNCHRONIZED_IO	1
				/* memory mapped files */
#define	_POSIX_MAPPED_FILES	1
				/* memory locking of whole address space */
#define	_POSIX_MEMLOCK		1
				/* memory locking address ranges */
#define	_POSIX_MEMLOCK_RANGE	1
				/* memory access protections */
#define	_POSIX_MEMORY_PROTECTION 1
				/* monotonic clock */
#define	_POSIX_MONOTONIC_CLOCK	200112L
				/* threads */
#define	_POSIX_THREADS		200112L
				/* semaphores */
#define	_POSIX_SEMAPHORES	0
				/* barriers */
#define	_POSIX_BARRIERS		200112L
				/* timers */
#define	_POSIX_TIMERS		200112L
				/* spin locks */
#define	_POSIX_SPIN_LOCKS	200112L
				/* read/write locks */
#define	_POSIX_READER_WRITER_LOCKS	200112L
				/* XPG4.2 shared memory */
#define	_XOPEN_SHM		0

/* access function */
#define	F_OK		0	/* test for existence of file */
#define	X_OK		0x01	/* test for execute or search permission */
#define	W_OK		0x02	/* test for write permission */
#define	R_OK		0x04	/* test for read permission */

/* whence values for lseek(2) */
#define	SEEK_SET	0	/* set file offset to offset */
#define	SEEK_CUR	1	/* set file offset to current plus offset */
#define	SEEK_END	2	/* set file offset to EOF plus offset */

#if defined(_NETBSD_SOURCE)
/* whence values for lseek(2); renamed by POSIX 1003.1 */
#define	L_SET		SEEK_SET
#define	L_INCR		SEEK_CUR
#define	L_XTND		SEEK_END
#endif

/* configurable pathname variables */
#define	_PC_LINK_MAX		 1
#define	_PC_MAX_CANON		 2
#define	_PC_MAX_INPUT		 3
#define	_PC_NAME_MAX		 4
#define	_PC_PATH_MAX		 5
#define	_PC_PIPE_BUF		 6
#define	_PC_CHOWN_RESTRICTED	 7
#define	_PC_NO_TRUNC		 8
#define	_PC_VDISABLE		 9
#define	_PC_SYNC_IO		10
#define	_PC_FILESIZEBITS	11

/* configurable system variables */
/*
 * XXX The value of _SC_CLK_TCK is embedded in <time.h>.
 * XXX The value of _SC_PAGESIZE is embedded in <sys/shm.h>.
 */
#define	_SC_ARG_MAX		 1
#define	_SC_CHILD_MAX		 2
#define	_O_SC_CLK_TCK		 3 /* Old version, always 100 */
#define	_SC_NGROUPS_MAX		 4
#define	_SC_OPEN_MAX		 5
#define	_SC_JOB_CONTROL		 6
#define	_SC_SAVED_IDS		 7
#define	_SC_VERSION		 8
#define	_SC_BC_BASE_MAX		 9
#define	_SC_BC_DIM_MAX		10
#define	_SC_BC_SCALE_MAX	11
#define	_SC_BC_STRING_MAX	12
#define	_SC_COLL_WEIGHTS_MAX	13
#define	_SC_EXPR_NEST_MAX	14
#define	_SC_LINE_MAX		15
#define	_SC_RE_DUP_MAX		16
#define	_SC_2_VERSION		17
#define	_SC_2_C_BIND		18
#define	_SC_2_C_DEV		19
#define	_SC_2_CHAR_TERM		20
#define	_SC_2_FORT_DEV		21
#define	_SC_2_FORT_RUN		22
#define	_SC_2_LOCALEDEF		23
#define	_SC_2_SW_DEV		24
#define	_SC_2_UPE		25
#define	_SC_STREAM_MAX		26
#define	_SC_TZNAME_MAX		27
#define	_SC_PAGESIZE		28
#define	_SC_PAGE_SIZE		_SC_PAGESIZE	/* 1170 compatibility */
#define	_SC_FSYNC		29
#define	_SC_XOPEN_SHM		30
#define	_SC_SYNCHRONIZED_IO	31
#define	_SC_IOV_MAX		32
#define	_SC_MAPPED_FILES	33
#define	_SC_MEMLOCK		34
#define	_SC_MEMLOCK_RANGE	35
#define	_SC_MEMORY_PROTECTION	36
#define	_SC_LOGIN_NAME_MAX	37
#define	_SC_MONOTONIC_CLOCK	38
#define	_SC_CLK_TCK		39 /* New, variable version */
#define	_SC_ATEXIT_MAX		40
#define	_SC_THREADS		41
#define	_SC_SEMAPHORES		42
#define	_SC_BARRIERS		43
#define	_SC_TIMERS		44
#define	_SC_SPIN_LOCKS		45
#define	_SC_READER_WRITER_LOCKS	46

/* configurable system strings */
#define	_CS_PATH		 1

#endif /* !_SYS_UNISTD_H_ */

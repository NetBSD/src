/*	$NetBSD: tpstats.h,v 1.1 2005/08/07 12:21:46 blymn Exp $	*/

/*
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from code donated to the NetBSD Foundation, Inc.
 * by Brett Lymn.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/time.h>
#include <sys/sched.h>
#include <unistd.h>

/* Tape drive entry to hold the information we're interested in. */
struct _tape {
	int		 *select;	/* Display stats for selected tapes. */
	char		**name;		/* Tape drive names (st0, etc). */
	u_int64_t	 *rxfer;	/* # of read transfers. */
	u_int64_t	 *wxfer;	/* # of write transfers. */
	u_int64_t	 *rbytes;	/* # of bytes read. */
	u_int64_t	 *wbytes;	/* # of bytes written. */
	struct timeval	 *time;		/* Time spent in tape i/o. */
};

extern struct _tape	cur_tape;
extern char		**tp_name;
extern int		*tp_select, tp_ndrive;

int	tpinit(int);
void	tpreadstats(void);
void	tpswap(void);



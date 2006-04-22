/*	$NetBSD: drvstats.h,v 1.1.2.2 2006/04/22 02:54:47 simonb Exp $	*/

/*
 * Copyright (c) 1996 John M. Vinopal
 * All rights reserved.
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
 *      This product includes software developed for the NetBSD Project
 *      by John M. Vinopal.
 * 4. The name of the author may not be used to endorse or promote products
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

/* poseur disk entry to hold the information we're interested in. */
struct _drive {
	int		 *select;	/* Display stats for selected disks. */
	char		**name;	/* Disk names (sd0, wd1, etc). */
	u_int64_t	 *rxfer;	/* # of read transfers. */
	u_int64_t	 *wxfer;	/* # of write transfers. */
	u_int64_t	 *seek;	/* # of seeks (currently unused). */
	u_int64_t	 *rbytes;	/* # of bytes read. */
	u_int64_t	 *wbytes;	/* # of bytes written. */
	struct timeval	 *time;	/* Time spent in disk i/o. */
	u_int64_t	  tk_nin;	/* TTY Chars in. */
	u_int64_t	  tk_nout;	/* TTY Chars out. */
	u_int64_t	  cp_time[CPUSTATES];	/* System timer ticks. */
	int	 	  cp_ncpu;		/* Number of cpu's */
	double		  cp_etime;		/* Elapsed time */
};

extern struct _drive	cur;
extern char		**dr_name;
extern int		*drv_select, ndrive;

int	drvinit(int);
void	cpureadstats(void);
void	drvreadstats(void);
void	tkreadstats(void);
void	cpuswap(void);
void	drvswap(void);
void	tkswap(void);

/*	$NetBSD: bugsyscalls.S,v 1.1 2002/02/27 21:02:27 scw Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>

#define	BSYS(num)	addi r10,0,num ; sc


/*
 * char bugsys_inchr(void);
 *
 * Blocks until a character is received from the console.
 * Returns character.
 */
ENTRY(bugsys_inchr)
	BSYS(0x0000)
	blr

/*
 * void bugsys_outchr(char ch)
 */
ENTRY(bugsys_outchr)
	BSYS(0x0020)
	blr

/*
 * int bugsys_instat(void);
 *
 * Returns zero if no characters are waiting to be read
 * from the console. (i.e. bugsys_inchr() would block).
 * Otherwise, it returns a non-zero value.
 */
ENTRY(bugsys_instat)
	BSYS(0x0002)
	li	r4,0x8
	and	r3,r3,r4
	blr

/*
 * int bugsys_dskrd(struct bug_diskio *dcmd);
 *
 * Read data from disk according to the parameters specified
 * in the `dcmd' data structure.
 *
 * Returns zero on success, non-zero on failure.
 */
ENTRY(bugsys_dskrd)
	BSYS(0x0010)
	li	r4,0x8
	and	r3,r3,r4
	blr

/*
 * int bugsys_dskwr(struct bug_diskio *dcmd);
 *
 * Write data to disk according to the parameters specified
 * in the `dcmd' data structure.
 *
 * Returns zero on success, non-zero on failure.
 */
ENTRY(bugsys_dskwr)
	BSYS(0x0011)
	li	r4,0x8
	and	r3,r3,r4
	blr

/*
 * int bugsys_netio(struct bug_netcmd *ncmd);
 *
 * Performs an I/O operation to/from a network device as
 * specified in the `ncmd' data structure.
 *
 * Returns zero on success, non-zero on failure.
 */
ENTRY(bugsys_netio)
	BSYS(0x001d)
	li	r4,0x8
	and	r3,r3,r4
	blr

/*
 * void bugsys_delay(int );
 *
 * Suspend processing for the specified number of milli-seconds
 */
ENTRY(bugsys_delay)
	BSYS(0x0043)
	blr

/*
 * struct bug_boardid *bugsys_brdid(void);
 *
 * Returns a pointer to the board-id structure.
 */
ENTRY(bugsys_brdid)
	BSYS(0x0070)
	blr

/*
 * struct bug_ioinquiry *bugsys_ioinq(void);
 *
 * Returns a pointer to the ioinq structure.
 */
ENTRY(bugsys_ioinq)
	BSYS(0x0120)
	blr

/*
 * void bugsys_rtc_rd(struct bug_rtc_rd *);
 *
 * Reads the current time from the RTC chip.
 */
ENTRY(bugsys_rtc_rd)
	BSYS(0x0053)
	blr

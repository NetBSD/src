/*	$NetBSD: satlinkreg.h,v 1.1 1997/07/13 19:09:49 hpeyerl Exp $	*/
/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Canada Connect Corp.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DEV_ISA_SATLINKREG_H_
#define	_DEV_ISA_SATLINKREG_H_

/*
 * Register definitions for the SatLink interface.
 */

#define	SATLINK_IOSIZE		0x20

#define	SATLINK_COMMAND		0x00	/* command register       */
#define	SATLINK_MFRID_L		0x1c	/* Manufacturer ID (low)  */
#define	SATLINK_MFRID_H		0x1b	/*                 (high) */
#define	SATLINK_GRPID		0x1d	/* Group ID               */
#define	SATLINK_USERID_L	0x1f	/* User ID (low)          */
#define	SATLINK_USERID_H	0x1e	/*         (high)         */
#define	SATLINK_SER_L		0x10	/* Serial number   (low)  */
#define	SATLINK_SER_M0		0x11	/*                   .    */
#define	SATLINK_SER_M1		0x12	/*                   .    */
#define	SATLINK_SER_H		0x13	/*                 (high) */

#define	SATLINK_CMD_RESET	0x0c	/* reset card             */

#endif /* _DEV_ISA_SATLINKREG_H_ */

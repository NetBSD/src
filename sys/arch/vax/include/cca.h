/*	$NetBSD: cca.h,v 1.1 2000/07/06 17:34:29 ragge Exp $	*/

/*
 * Copyright (c) 2000 Ludd, University of Lule}, Sweden. All rights reserved.
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
 *      This product includes software developed at Ludd, University of
 *      Lule}, Sweden and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Console Communication Area (CCA) layout description.
 * OBS! This is incomplete and should be filled out if someone
 * find docs for it.
 */

struct cca {
	struct	cca *cca_base;	/* Physical base address of block */
	short 	cca_size;	/* Size of this struct */
	short	cca_id;		/* 'CC' */
	char	cca_maxcpu;	/* max number of CPUs */
	char	cca_cksum;	/* Checksum of all earlier bytes */
	char	cca_flags;
	char	cca_revision;

	u_int64_t cca_ready;	/* Data ready? */
	u_int64_t cca_console;	/* Processors in console mode */
	u_int64_t cca_enabled;	/* enabled/disabled */
	long	cca_bitmapsz;	/* Size of memory bitmap */
	long	cca_bitmap;	/* Address of memory bitmap */
	long	cca_bmcksum;	/* Bitmap checksum */
	long	cca_bootdev;	/* Node numbers */
	u_int64_t cca_starting;	/* Processors currently starting */
	u_int64_t cca_restart;	/* Processors currently restarting */
	long	cca_pad1[3];
	u_int64_t cca_halted;	/* Processors currently halted bny user */
	char	cca_sernum[8];	/* Serial number */
	char	cca_revs[16][8];/* CPU revisions */
	u_int64_t cca_vecenab;	/* Processors with enabled vector processors */
	u_int64_t cca_vecwork;	/* Processors with working vector processors */
	long	cca_vecrevs[16];/* Vector processor revisions */
	char	cca_pad2[208];
/* Inter-CPU communication structs */
	struct {
		char	cc_flags;	/* Status flags */
		char	cc_to;		/* Node sending to */
		char	cc_from;	/* Node sending from */
		char	cc_pad;
		char	cc_txlen;	/* Length of transmit message */
		char	cc_rxlen;	/* Length of receive message */
		char	cc_unbuf;
		char	cc_txbuf[80];	/* Transmit buffer */
		char	cc_rxbuf[80];	/* Receive buffer */
	} cca_cc[64];
};

#ifdef _KERNEL
extern	struct cca *cca;
#endif

/*	$NetBSD: cca.h,v 1.2.48.1 2017/12/03 11:36:47 jdolecek Exp $	*/

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
	struct cca *cca_base;	/* Physical base address of block */
	uint16_t cca_size;	/* Size of this struct */
	uint16_t cca_id;	/* 'CC' */
	uint8_t  cca_maxcpu;	/* max number of CPUs */
	uint8_t  cca_cksum;	/* Checksum of all earlier bytes */
	uint8_t  cca_flags;
	uint8_t  cca_revision;

	uint64_t cca_ready;	/* Data ready? */
	uint64_t cca_console;	/* Processors in console mode */
	uint64_t cca_enabled;	/* enabled/disabled */
	uint32_t cca_bitmapsz;	/* Size of memory bitmap */
	uint32_t cca_bitmap;	/* Address of memory bitmap */
	uint32_t cca_bmcksum;	/* Bitmap checksum */
	uint32_t cca_bootdev;	/* Node numbers */
	uint64_t cca_starting;	/* Processors currently starting */
	uint64_t cca_restart;	/* Processors currently restarting */
	uint32_t cca_pad1[3];
	uint64_t cca_halted;	/* Processors currently halted bny user */
	uint8_t  cca_sernum[8];	/* Serial number */
	uint8_t  cca_revs[16][8];/* CPU revisions */
	uint64_t cca_vecenab;	/* Processors with enabled vector processors */
	uint64_t cca_vecwork;	/* Processors with working vector processors */
	uint32_t cca_vecrevs[16];/* Vector processor revisions */
	uint8_t  cca_pad2[208];
/* Inter-CPU communication structs */
	struct {
		uint8_t	cc_flags;	/* Status flags */
		uint8_t	cc_to;		/* Node sending to */
		uint8_t	cc_from;	/* Node sending from */
		uint8_t	cc_pad;
		uint8_t	cc_txlen;	/* Length of transmit message */
		uint8_t	cc_rxlen;	/* Length of receive message */
		uint8_t	cc_unbuf;
		char	cc_txbuf[80];	/* Transmit buffer */
		char	cc_rxbuf[80];	/* Receive buffer */
	} cca_cc[64];
};

#ifdef _KERNEL
extern	struct cca *cca;
#endif

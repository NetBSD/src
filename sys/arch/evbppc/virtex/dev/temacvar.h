/* 	$NetBSD: temacvar.h,v 1.2 2019/04/11 14:38:05 kamil Exp $	*/

/*
 * Copyright (c) 2006 Jachym Holecek
 * All rights reserved.
 *
 * Written for "DSP&FPGA Custom Design".
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
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

#ifndef _VIRTEX_DEV_TEMACVAR_H_
#define _VIRTEX_DEV_TEMACVAR_H_

/* TEMAC specific fields in Rx CDMAC descriptor. */
#define desc_rxstat 		desc_user3
#define RXSTAT_GOOD 		0x80000000 	/* Good frame */
#define RXSTAT_BAD 		0x40000000 	/* Bad frame */
#define RXSTAT_FLOW 		0x20000000 	/* Flow control frame */
#define RXSTAT_VLAN 		0x10000000 	/* Contains VLAN tag */
#define RXSTAT_BADCRC 		0x08000000 	/* Failed FCS validation */
#define RXSTAT_TRUNC 		0x04000000 	/* Truncated, FIFO overflow */
#define RXSTAT_RAWCSUM_MASK 	0x0000ffff 	/* Precalculated raw csum */

#define RXSTAT_SICK 		(RXSTAT_BAD | RXSTAT_FLOW | RXSTAT_BADCRC | \
				 RXSTAT_TRUNC)

#define desc_rxsize 		desc_user4
#define RXSIZE_MASK 		0x0000ffff 	/* Length of frame in bytes */

/* TEMAC specific fields in Tx CDMAC descriptor. */
#define desc_txcon 		desc_user0 	/* Careful: overlayed w/ stat */
#define TXCON_TCP 		0x0002 		/* TCP segment, UDP otherwise */
#define TXCON_CSUM 		0x0001 		/* Calculate checksum in hw */

#define desc_txcsum 		desc_user1
#define CSUM_INSERT_MASK 	0x003ff 	/* Offset where to insert cs */
#define CSUM_START_MASK 	0x003ff 	/* Offset where calc starts */
#define CSUM_START_OFFS 	16

#define desc_txcsumini 		desc_user2 	/* Initial value for cs calc */
#define CSUMINI_MASK 		0xffff

#endif /* _VIRTEX_DEV_TEMACVAR_H_ */

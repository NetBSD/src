/*	$NetBSD: cscreg.h,v 1.1.4.2 2001/10/05 22:27:56 reinoud Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Scott Stevens.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Cumana SCSI-2 with FAS216 SCSI interface hardware description.
 */

#ifndef _CSCREG_H_
#define _CSCREG_H_

#include <acorn32/podulebus/sfasvar.h>

typedef volatile unsigned short vu_short;

typedef struct csc_regmap {
	sfas_regmap_t	FAS216;
	vu_char		*status0;
	vu_char		*alatch;
	vu_short	*dack;
} csc_regmap_t;
typedef csc_regmap_t *csc_regmap_p;

/*
 * Register information
 */
#define CSC_STATUS0		0x0000
#define CSC_ALATCH		0x0014
#define CSC_DACK		0x0200
#define CSC_FAS_OFFSET_BASE	0x0300
#define CSC_FAS_OFFSET_TCL	0x00
#define CSC_FAS_OFFSET_TCM	0x04
#define CSC_FAS_OFFSET_FIFO	0x08
#define CSC_FAS_OFFSET_COMMAND	0x0c
#define CSC_FAS_OFFSET_DESTID	0x10
#define CSC_FAS_OFFSET_TIMEOUT	0x14
#define CSC_FAS_OFFSET_PERIOD	0x18
#define CSC_FAS_OFFSET_OFFSET	0x1c
#define CSC_FAS_OFFSET_CONFIG1	0x20
#define CSC_FAS_OFFSET_CLKCONV	0x24
#define CSC_FAS_OFFSET_TEST	0x28
#define CSC_FAS_OFFSET_CONFIG2	0x2c
#define CSC_FAS_OFFSET_CONFIG3	0x30
#define CSC_FAS_OFFSET_TCH	0x38
#define CSC_FAS_OFFSET_FIFOBOT	0x3c

#define CSC_STATUS0_INT		0x01
#define CSC_STATUS0_DREQ	0x02
#define CSC_STATUS0_EDOUT	0x04
#define CSC_STATUS0_LATCHED	0x08

#define CSC_ALATCH_DEFS_P7	0x01
#define CSC_ALATCH_DEFS_INTEN	0x02
#define CSC_ALATCH_DEFS_TERM	0x04
#define CSC_ALATCH_DEFS_RSVD	0x08
#define CSC_ALATCH_DEFS_PROG	0x10
#define CSC_ALATCH_DEFS_DMA32	0x20
#define CSC_ALATCH_DEFS_DMAEN	0x40
#define CSC_ALATCH_DEFS_DMADIR	0x80

#endif

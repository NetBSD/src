/*	$NetBSD: disklabel.h,v 1.4 2004/07/27 22:14:22 jkunz Exp $	*/

/*	$OpenBSD: disklabel.h,v 1.5 2000/07/05 22:37:22 mickey Exp $	*/

/*
 * Copyright (c) 1994 Christopher G. Demetriou
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
 *      This product includes software developed by Christopher G. Demetriou.
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

#ifndef _MACHINE_DISKLABEL_H_
#define _MACHINE_DISKLABEL_H_

#define	LABELSECTOR		1		/* sector containing label */
#define	LABELOFFSET		0		/* offset of label in sector */

#define	MAXPARTITIONS		16		/* number of partitions */
#define	RAW_PART		2		/* raw partition: xx?c */

/*
 * volume header for "LIF" format volumes
 */
struct	lifvol {
	uint16_t	vol_id;
	uint8_t		vol_label[6];
	uint32_t	vol_addr;
	uint16_t	vol_oct;
	uint16_t	vol_dummy;

	uint32_t	vol_dirsize;
	uint16_t	vol_version;
	uint16_t	vol_zero;
	uint32_t	vol_number;
	uint32_t	vol_lastvol;

	uint32_t	vol_length;
	uint8_t		vol_toc[6];
	uint8_t		vol_dummy1[198];

	uint32_t	ipl_addr;
	uint32_t	ipl_size;
	uint32_t	ipl_entry;

	uint32_t	vol_dummy2;
};

struct	lifdir {
	uint8_t		dir_name[10];
	uint16_t	dir_type;
	uint32_t	dir_addr;
	uint32_t	dir_length;
	uint8_t		dir_toc[6];
	uint16_t	dir_flag;
	uint32_t	dir_implement;
};

struct lif_load {
	int address;
	int count;
};

#define LIF_VOL_ID	0x8000
#define LIF_VOL_OCT	0x1000
#define LIF_DIR_SWAP	0x5243
#define	LIF_DIR_FS	0xcd38
#define	LIF_DIR_IOMAP	0xcd60
#define	LIF_DIR_HPUX	0xcd80
#define	LIF_DIR_ISL	0xce00
#define	LIF_DIR_PAD	0xcffe
#define	LIF_DIR_AUTO	0xcfff
#define	LIF_DIR_EST	0xd001
#define	LIF_DIR_TYPE	0xe942

#define LIF_DIR_FLAG	0x8001	/* dont ask me! */
#define	LIF_SECTSIZE	256

#define LIF_NUMDIR	8

#define LIF_VOLSTART	0
#define LIF_VOLSIZE	sizeof(struct lifvol)
#define LIF_DIRSTART	2048
#define LIF_DIRSIZE	(LIF_NUMDIR * sizeof(struct lifdir))
#define LIF_FILESTART	4096

#define	btolifs(b)	(((b) + (LIF_SECTSIZE - 1)) / LIF_SECTSIZE)
#define	lifstob(s)	((s) * LIF_SECTSIZE) 
#define	lifstodb(s)	((s) * LIF_SECTSIZE / DEV_BSIZE)

#include <sys/dkbad.h>
struct cpu_disklabel {
	struct lifvol lifvol;
	struct lifdir lifdir[LIF_NUMDIR];
	struct dkbad bad;			/* To make wd(4) happy */
};

#endif /* _MACHINE_DISKLABEL_H_ */

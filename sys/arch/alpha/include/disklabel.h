/* $NetBSD: disklabel.h,v 1.6 2000/03/13 23:52:26 soren Exp $ */

/*
 * Copyright (c) 1994, 1999 Christopher G. Demetriou
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
 *      This product includes software developed by Christopher G. Demetriou
 *      for the NetBSD Project.
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

#define	LABELSECTOR	0			/* sector containing label */
#define	LABELOFFSET	64			/* offset of label in sector */
#define	MAXPARTITIONS	8			/* number of partitions */
#define	RAW_PART	2			/* raw partition: xx?c */

#include <sys/dkbad.h>

/* Just a dummy */
struct cpu_disklabel {
	struct dkbad bad;			/* bad-sector information */
};

/*
 * Alpha (disk, but also tape) Boot Block.  See Section (III) 3.6.1 of the
 * Alpha Architecture Reference Manual.
 */

struct boot_block {
	u_int64_t bb_data[63];		/* data (disklabel, also as below) */
	u_int64_t bb_cksum;		/* checksum of the boot block,
					 * taken as u_int64_t's
					 */
};
#define	bb_secsize	bb_data[60]	/* secondary size (blocks) */
#define	bb_secstart	bb_data[61]	/* secondary start (blocks) */
#define	bb_flags	bb_data[62]	/* unknown flags (set to zero) */

#define	BOOT_BLOCK_OFFSET	0	/* offset of boot block. */
#define	BOOT_BLOCK_BLOCKSIZE	512	/* block size for sec. size/start,
					 * and for boot block itself
					 */

#define	CHECKSUM_BOOT_BLOCK(bb,cksum)					\
	do {								\
		const struct boot_block *_bb = (bb);			\
		u_int64_t _cksum;					\
		int _i;							\
									\
		_cksum = 0;						\
		for (_i = 0;						\
		    _i < (sizeof _bb->bb_data / sizeof _bb->bb_data[0]); \
		    _i++)						\
			_cksum += _bb->bb_data[_i];			\
		*(cksum) = _cksum;					\
	} while (0)

#endif /* _MACHINE_DISKLABEL_H_ */

/*	$NetBSD: tospart.h,v 1.1.1.1 1995/03/26 07:12:08 leo Exp $	*/

/*
 * Copyright (c) 1995 Leo Weppelman.
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
 *      This product includes software developed by Leo Weppelman.
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

#ifndef _MACHINE_TOSPART_H
#define _MACHINE_TOSPART_H
/*
 * Format of GEM root sector
 */
typedef struct gem_part {
	u_char	p_flg;		/* bit 0 is in-use flag			*/
	u_char	p_id[3];	/* id: GEM, BGM, XGM, UNX, MIX		*/
	u_long	p_st;		/* block where partition starts		*/
	u_long	p_size;		/* partition size			*/
} GEM_PART;

#define	NGEM_PARTS	4	/* Max. partition infos in root sector	*/

typedef struct gem_root {
	u_char		fill[0x1c2];	/* Filler, can be boot code	*/
	u_long		hd_siz;		/* size of entire volume	*/
	GEM_PART	parts[NGEM_PARTS];/* see above			*/
	u_long		bsl_st;		/* start of bad-sector list	*/
	u_long		bsl_cnt;	/* # of blocks in bad-sect. list*/
} GEM_ROOT;

#define	TOS_BSIZE	512	/* TOS blocksize			*/
#define	TOS_BBLOCK	0	/* TOS Bootblock			*/
#define	TOS_MAXPART	32	/* Maximum number of partitions we read	*/

#endif /* _MACHINE_TOSPART_H */

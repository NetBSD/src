/*	$NetBSD: installboot.h,v 1.12 2002/05/15 09:44:55 lukem Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn of Wasabi Systems.
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

#ifndef	_INSTALLBOOT_H
#define	_INSTALLBOOT_H

#if HAVE_CONFIG_H
#include "config.h"
#include "../../sys/sys/bootblock.h"
#else
#include <sys/bootblock.h>
#endif

#include <sys/stat.h>
#include <stdint.h>

typedef enum {
				/* flags from global options */
	IB_VERBOSE =	1<<0,		/* verbose operation */
	IB_NOWRITE =	1<<1,		/* don't write */
	IB_CLEAR =	1<<2,		/* clear boot block */

				/* flags from -o options */
	IB_ALPHASUM =	1<<8,		/* set Alpha checksum */
	IB_APPEND =	1<<9,		/* clear boot block */
	IB_SUNSUM =	1<<10,		/* set Sun checksum */
	IB_STAGE1START=	1<<11,		/* start block for stage 1 provided */
	IB_STAGE2START=	1<<12,		/* start block for stage 2 provided */
} ib_flags;

typedef struct {
	ib_flags	 flags;		/* flags (see above) */
	struct ib_mach	*machine;	/* machine details (see below) */
	struct ib_fs	*fstype;	/* file system details (see below) */
	const char	*filesystem;	/* name of target file system */
	int		 fsfd;		/*  open fd to filesystem */
	struct stat	 fsstat;	/*  fstat(2) of fsfd */
	const char	*stage1;	/* name of stage1 bootstrap */
	int		 s1fd;		/*  open fd to stage1 */
	struct stat	 s1stat;	/*  fstat(2) of s1fd */
	uint32_t	 s1start;	/*  start block of stage1 */
	const char	*stage2;	/* name of stage2 bootstrap */
	uint32_t	 s2start;	/*  start block of stage2 */
} ib_params;

typedef struct {
	uint32_t	block;
	uint32_t	blocksize;
} ib_block;

struct ib_mach {
	const char	*name;
	int		(*parseopt)	(ib_params *, const char *);
	int		(*setboot)	(ib_params *);
	int		(*clearboot)	(ib_params *);
};

struct ib_fs {
		/* compile time parameters */
	const char	*name;
	int		(*match)	(ib_params *);
	int		(*findstage2)	(ib_params *, uint32_t *, ib_block *);
		/* run time fs specific parameters */
	uint32_t	 blocksize;
	uint32_t	 needswap;
};

struct bbinfo_params {
	const char	*magic;
	uint32_t	offset;
	uint32_t	blocksize;
	uint32_t	maxsize;
	uint32_t	headeroffset;
	int		littleendian;
};

extern struct ib_mach	machines[];
extern struct ib_fs	fstypes[];

	/* installboot.c */
int		parseoptionflag(ib_params *, const char *, ib_flags);
uint16_t	compute_sunsum(const uint16_t *);
int		set_sunsum(ib_params *, uint16_t *, uint16_t);
int		no_parseopt(ib_params *, const char *);
int		no_setboot(ib_params *);
int		no_clearboot(ib_params *);

	/* bbinfo.c */
int		shared_bbinfo_clearboot(ib_params *, struct bbinfo_params *);
int		shared_bbinfo_setboot(ib_params *, struct bbinfo_params *,
		    int (*)(ib_params *, struct bbinfo_params *, char *));

	/* fstypes.c */
int		hardcode_stage2(ib_params *, uint32_t *, ib_block *);
int		ffs_match(ib_params *);
int		ffs_findstage2(ib_params *, uint32_t *, ib_block *);
int		raw_match(ib_params *);
int		raw_findstage2(ib_params *, uint32_t *, ib_block *);

	/* machines.c */
int		alpha_parseopt(ib_params *, const char *);
int		alpha_setboot(ib_params *);
int		alpha_clearboot(ib_params *);
int		macppc_setboot(ib_params *);
int		macppc_clearboot(ib_params *);
int		pmax_parseopt(ib_params *, const char *);
int		pmax_setboot(ib_params *);
int		pmax_clearboot(ib_params *);
int		sparc_setboot(ib_params *);
int		sparc_clearboot(ib_params *);
int		sparc64_setboot(ib_params *);
int		sparc64_clearboot(ib_params *);
int		sun68k_setboot(ib_params *);
int		sun68k_clearboot(ib_params *);
int		vax_parseopt(ib_params *, const char *);
int		vax_setboot(ib_params *);
int		vax_clearboot(ib_params *);

#endif	/* _INSTALLBOOT_H */

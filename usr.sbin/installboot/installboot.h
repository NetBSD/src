/*	$NetBSD: installboot.h,v 1.5 2002/04/19 07:08:52 lukem Exp $	*/

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
#endif

typedef enum {
				/* flags from global options */
	IB_VERBOSE =	1<<0,		/* verbose operation */
	IB_NOWRITE =	1<<1,		/* don't write */
	IB_CLEAR =	1<<2,		/* clear boot block */
	IB_STARTBLOCK =	1<<3,		/* start block supplied */

				/* flags from -o options */
	IB_ALPHASUM =	1<<8,		/* set Alpha checksum */
	IB_APPEND =	1<<9,		/* clear boot block */
	IB_SUNSUM =	1<<10,		/* set Sun checksum */
} ib_flags;

typedef struct {
	ib_flags	 flags;
	struct ib_mach	*machine;
	struct ib_fs	*fstype;
	const char	*filesystem;
	int		 fsfd;
	const char	*stage1;
	int		 s1fd;
	const char	*stage2;
	uint32_t	 startblock;
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
	const char	*name;
	int		(*match)	(ib_params *);
	int		(*findstage2)	(ib_params *, uint32_t *, ib_block *);
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

	/* fstypes.c */
int		ffs_match(ib_params *);
int		ffs_findstage2(ib_params *, uint32_t *, ib_block *);

	/* machines.c */
int		alpha_parseopt(ib_params *, const char *);
int		alpha_setboot(ib_params *);
int		alpha_clearboot(ib_params *);
int		pmax_parseopt(ib_params *, const char *);
int		pmax_setboot(ib_params *);
int		pmax_clearboot(ib_params *);
int		sparc64_setboot(ib_params *);
int		sparc64_clearboot(ib_params *);
int		vax_parseopt(ib_params *, const char *);
int		vax_setboot(ib_params *);
int		vax_clearboot(ib_params *);

#endif	/* _INSTALLBOOT_H */

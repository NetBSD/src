/*	$NetBSD: installboot.h,v 1.2 2002/04/04 17:53:04 bjh21 Exp $	*/

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
	const char	*fstype;	// XXX replace with struct *?
	const char	*filesystem;
	int		 fsfd;
	const char	*bootblock;
	int		 bbfd;
	long		 startblock;
} ib_params;

struct ib_mach {
	const char	*name;
	int		(*parseopt)	(ib_params *, const char *);
	int		(*setboot)	(ib_params *);
	int		(*clearboot)	(ib_params *);
};


extern struct ib_mach machines[];

int		parseoptionflag(ib_params *, const char *, ib_flags);
u_int16_t	compute_sunsum(const u_int16_t *);
int		set_sunsum(ib_params *, u_int16_t *, u_int16_t);

#endif	/* _INSTALLBOOT_H */

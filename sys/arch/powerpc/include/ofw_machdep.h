/* $NetBSD: ofw_machdep.h,v 1.6 2022/11/24 00:07:49 macallan Exp $ */

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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

#ifndef _POWERPC_OFW_MACHDEP_H_
#define _POWERPC_OFW_MACHDEP_H_

#ifdef _KERNEL
#include <machine/powerpc.h>

/*
 * The general format of an OpenFirmware virtual translation record is:
 *
 *	cell(s)		virt
 *	cell(s)		size
 *	cell(s)		phys
 *	cell		mode
 *
 * "mode" contains PTE WIMG bits.
 *
 * We define this structure to describe these translations that's independent
 * of the number of cells each field consumes.
 */
struct OF_translation {
	vaddr_t		virt;
	vsize_t		size;
	paddr_t		phys;
	uint32_t	mode;
};

#define	OFW_MAX_TRANSLATIONS	48

extern bool ofwbootcons_suppress; /* suppress OF console I/O */

extern int ofw_chosen;		/* cached handle for "/chosen" */
extern struct OF_translation ofw_translations[OFW_MAX_TRANSLATIONS];

void	ofw_bootstrap(void);
void 	ofprint(const char *, ...);

#endif /* _KERNEL */

#endif /* _POWERPC_OFW_MACHDEP_H_ */

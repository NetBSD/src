/*	$NetBSD: undefined.h,v 1.12.64.1 2019/06/10 22:05:54 christos Exp $	*/

/*
 * Copyright (c) 1995-1996 Mark Brinicombe.
 * Copyright (c) 1995 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * undefined.h
 *
 * Undefined instruction types, symbols and prototypes
 *
 * Created      : 08/02/95
 */


#ifndef _ARM_UNDEFINED_H_
#define _ARM_UNDEFINED_H_
#ifdef _KERNEL

#include <sys/queue.h>

typedef int (*undef_handler_t)(unsigned int, unsigned int, trapframe_t *, int);

/*
 * Enumeration of coprocessor numbers.  Values may be duplicated
 * (the iWMMX coprocessor clashes with the FPA, for example), but
 * keep this table in numeric order.
 */
enum arm_coprocs {
	FPA_COPROC = 1,
	FPA_COPROC2 = 2,
	VFP_COPROC = 10,
	VFP_COPROC2 = 11,
	DEBUG_COPROC = 14,
	SYSTEM_COPROC = 15,
	/*
	 *The following are not really co-processors, but are on the end
	 * of the unknown instruction table for each coproc.
	 */
	CORE_UNKNOWN_HANDLER = 16,
#ifdef THUMB_CODE
	THUMB_UNKNOWN_HANDLER = 17,
#endif
	NUM_UNKNOWN_HANDLERS	/* Last entry */
};

/* Prototypes for undefined.c */

void *install_coproc_handler(int, undef_handler_t);
void remove_coproc_handler(void *);
void undefined_init(void);

/*
 * XXX Stuff below here is for use before malloc() is available.  Most code
 * shouldn't use it.
 */

struct undefined_handler {
	LIST_ENTRY(undefined_handler) uh_link;
	undef_handler_t uh_handler;
};

/*
 * Handlers installed using install_coproc_handler_static shouldn't be
 * removed.
 */
void install_coproc_handler_static(int, struct undefined_handler *);

/* Calls up to undefined.c from trap handlers */
void undefinedinstruction(struct trapframe *);

#endif

/* End of undefined.h */

#endif /* _ARM_UNDEFINED_H_ */

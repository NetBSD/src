/*	$NetBSD: pthread_md.c,v 1.4 2003/07/26 18:33:06 mrg Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread_md.c,v 1.4 2003/07/26 18:33:06 mrg Exp $");

#define	__PTHREAD_SIGNAL_PRIVATE

#include <string.h>
#include <assert.h>

#include "pthread.h"
#include "pthread_int.h"

/*
 * We only need this to know how much data to copy.
 */
static const int exframesize[] = {
	FMT0SIZE,	/* type 0 - normal (68020/030/040/060) */
	FMT1SIZE,	/* type 1 - throwaway (68020/030/040) */
	FMT2SIZE,	/* type 2 - normal 6-word (68020/030/040/060) */
	FMT3SIZE,	/* type 3 - FP post-instruction (68040/060) */
	FMT4SIZE,	/* type 4 - access error/fp disabled (68060) */
	-1, -1,		/* type 5-6 - undefined */
	FMT7SIZE,	/* type 7 - access error (68040) */
	58,		/* type 8 - bus fault (68010) */ 
	FMT9SIZE,	/* type 9 - coprocessor mid-instruction (68020/030) */
	FMTASIZE,	/* type A - short bus fault (68020/030) */
	FMTBSIZE,	/* type B - long bus fault (68020/030) */ 
	-1, -1, -1, -1	/* type C-F - undefined */
};
#define	_SIGSTATE_EXFRAMESIZE(fmt)	exframesize[(fmt)]

/*
 * m68k signal contexts are quite screwey, so we have to go through
 * a fair bit of effort to convert them.
 */
void
pthread__ucontext_to_sigcontext(const sigset_t *mask, ucontext_t *uc,
    struct pthread__sigcontext *psc)
{

	uc->uc_sigmask = *mask;

	_MCONTEXT_TO_SIGSTATE(uc, &psc->psc_state);	/* must be first */
	_UCONTEXT_TO_SIGCONTEXT(uc, &psc->psc_context);

	psc->psc_context.sc_ap = (int) &psc->psc_state;
}

void
pthread__sigcontext_to_ucontext(const struct pthread__sigcontext *psc,
    ucontext_t *uc)
{

	_SIGSTATE_TO_MCONTEXT(&psc->psc_state, uc);	/* must be first */
	_SIGCONTEXT_TO_UCONTEXT(&psc->psc_context, uc);

	uc->uc_flags &= ~_UC_SIGMASK;
}

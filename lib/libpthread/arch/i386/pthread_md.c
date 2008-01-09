/*	$NetBSD: pthread_md.c,v 1.3.24.1 2008/01/09 01:36:42 matt Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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
__RCSID("$NetBSD: pthread_md.c,v 1.3.24.1 2008/01/09 01:36:42 matt Exp $");

#include <sys/param.h>
#include <sys/sysctl.h>

#include <machine/sysarch.h>
#include <machine/cpu.h>

#include "pthread.h"
#include "pthread_int.h"

int (*_md_getcontext_u)(ucontext_t *);
int (*_md_setcontext_u)(const ucontext_t *);
int (*_md_swapcontext_u)(ucontext_t *, const ucontext_t *);

/*
 * Initialize the function pointers for get/setcontext to the
 * old-fp-save (s87) or new-fxsave (xmm) FP routines.
 */
void
pthread__i386_init(void)
{
	int mib[2];
	size_t len;
	int use_fxsave;

	mib[0] = CTL_MACHDEP;
	mib[1] = CPU_OSFXSR;

	len = sizeof(use_fxsave);
	sysctl(mib, 2, &use_fxsave, &len, NULL, 0);

	if (use_fxsave) {
		_md_getcontext_u = _getcontext_u_xmm;
		_md_setcontext_u = _setcontext_u_xmm;
		_md_swapcontext_u = _swapcontext_u_xmm;
	} else {
		_md_getcontext_u = _getcontext_u_s87;
		_md_setcontext_u = _setcontext_u_s87;
		_md_swapcontext_u = _swapcontext_u_s87;
	}

}

void
pthread__threadreg_set(pthread_t self)
{

	sysarch(I386_SET_GSBASE, &self);
}

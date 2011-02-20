/*	$NetBSD: spl_stubs.c,v 1.3 2011/02/20 16:38:13 rmind Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: spl_stubs.c,v 1.3 2011/02/20 16:38:13 rmind Exp $");

#define __INTR_PRIVATE

#include <sys/param.h>

#include <mips/cache.h>
#include <mips/intr.h>
#include <mips/locore.h>

int	splhigh(void)		__section(".stub");
int	splhigh_noprof(void)	__section(".stub");
int	splsched(void)		__section(".stub");
int	splvm(void)		__section(".stub");
int	splsoftserial(void)	__section(".stub");
int	splsoftnet(void)	__section(".stub");
int	splsoftbio(void)	__section(".stub");
int	splsoftclock(void)	__section(".stub");
int	splraise(int)		__section(".stub");
void	splx(int)		__section(".stub");
void	splx_noprof(int)	__section(".stub");
void	spl0(void)		__section(".stub");
int	splintr(uint32_t *)	__section(".stub");
void	_setsoftintr(uint32_t)	__section(".stub");
void	_clrsoftintr(uint32_t)	__section(".stub");
void	splcheck(void)		__section(".stub");

int
splhigh(void)
{
	return (*mips_splsw.splsw_splhigh)();
}

int
splhigh_noprof(void)
{
	return (*mips_splsw.splsw_splhigh_noprof)();
}

int
splsched(void)
{
	return (*mips_splsw.splsw_splsched)();
}

int
splvm(void)
{
	return (*mips_splsw.splsw_splvm)();
}

int
splsoftserial(void)
{
	return (*mips_splsw.splsw_splsoftserial)();
}

int
splsoftnet(void)
{
	return (*mips_splsw.splsw_splsoftnet)();
}

int
splsoftbio(void)
{
	return (*mips_splsw.splsw_splsoftbio)();
}

int
splsoftclock(void)
{
	return (*mips_splsw.splsw_splsoftclock)();
}

void
spl0(void)
{
	(*mips_splsw.splsw_spl0)();
}

void
splx(int s)
{
	(*mips_splsw.splsw_splx)(s);
}

void
splx_noprof(int s)
{
	(*mips_splsw.splsw_splx_noprof)(s);
}

int
splraise(int s)
{
        return (*mips_splsw.splsw_splraise)(s);
}

int
splintr(uint32_t *p)
{
	return (*mips_splsw.splsw_splintr)(p);
}

void
_setsoftintr(uint32_t m)
{
	(*mips_splsw.splsw__setsoftintr)(m);
}

void
_clrsoftintr(uint32_t m)
{
	(*mips_splsw.splsw__clrsoftintr)(m);
}

void
splcheck(void)
{
	(*mips_splsw.splsw_splcheck)();
}

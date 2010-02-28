/* $NetBSD: spl_stubs.c,v 1.1.2.1 2010/02/28 03:23:06 matt Exp $ */
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

__KERNEL_RCSID(0, "$NetBSD: spl_stubs.c,v 1.1.2.1 2010/02/28 03:23:06 matt Exp $");

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

#define	J_SPLHIGH	0
int
splhigh(void)
{
	return (*mips_splsw.splsw_splhigh)();
}

#define	J_SPLHIGH_NOPROF	(J_SPLHIGH+1)
int
splhigh_noprof(void)
{
	return (*mips_splsw.splsw_splhigh_noprof)();
}

#define	J_SPLSCHED		(J_SPLHIGH_NOPROF+1)
int
splsched(void)
{
	return (*mips_splsw.splsw_splsched)();
}

#define	J_SPLVM			(J_SPLSCHED+1)
int
splvm(void)
{
	return (*mips_splsw.splsw_splvm)();
}

#define	J_SPLSOFTSERIAL		(J_SPLVM+1)
int
splsoftserial(void)
{
	return (*mips_splsw.splsw_splsoftserial)();
}

#define	J_SPLSOFTNET		(J_SPLSOFTSERIAL+1)
int
splsoftnet(void)
{
	return (*mips_splsw.splsw_splsoftnet)();
}

#define	J_SPLSOFTBIO		(J_SPLSOFTNET+1)
int
splsoftbio(void)
{
	return (*mips_splsw.splsw_splsoftbio)();
}

#define	J_SPLSOFTCLOCK		(J_SPLSOFTBIO+1)
int
splsoftclock(void)
{
	return (*mips_splsw.splsw_splsoftclock)();
}

#define	J_SPL0			(J_SPLSOFTCLOCK+1)
void
spl0(void)
{
	(*mips_splsw.splsw_spl0)();
}

#define	J_SPLX			(J_SPL0+1)
void
splx(int s)
{
	(*mips_splsw.splsw_splx)(s);
}

#define	J_SPLX_NOPROF		(J_SPLX+1)
void
splx_noprof(int s)
{
	(*mips_splsw.splsw_splx_noprof)(s);
}

#define	J_SPLRAISE		(J_SPLX_NOPROF+1)
int
splraise(int s)
{
        return (*mips_splsw.splsw_splraise)(s);
}

#define	J_SPLINTR		(J_SPLRAISE+1)
int
splintr(uint32_t *p)
{
	return (*mips_splsw.splsw_splintr)(p);
}

#define	J_SETSOFTINTR		(J_SPLINTR+1)
void
_setsoftintr(uint32_t m)
{
	(*mips_splsw.splsw__setsoftintr)(m);
}

#define	J_CLRSOFTINTR		(J_SETSOFTINTR+1)
void
_clrsoftintr(uint32_t m)
{
	(*mips_splsw.splsw__clrsoftintr)(m);
}

#define	J_SPLMAX		(J_CLRSOFTINTR+1)

#if 0
#define	offsetofsplsw(x)	(offsetof(struct splsw, x) / sizeof(uint32_t))
static uint32_t splreal[J_SPLMAX] = {
	[J_SPLHIGH] =		offsetofsplsw(splsw_splhigh),
	[J_SPLHIGH_NOPROF] =	offsetofsplsw(splsw_splhigh_noprof),
	[J_SPLSCHED] =		offsetofsplsw(splsw_splsched),
	[J_SPLVM] =		offsetofsplsw(splsw_splvm),
	[J_SPLSOFTSERIAL] =	offsetofsplsw(splsw_splsoftserial),
	[J_SPLSOFTNET] =	offsetofsplsw(splsw_splsoftnet),
	[J_SPLSOFTBIO] =	offsetofsplsw(splsw_splsoftbio),
	[J_SPLSOFTCLOCK] =	offsetofsplsw(splsw_splsoftclock),
	[J_SPL0] =		offsetofsplsw(splsw_spl0),
	[J_SPLX] =		offsetofsplsw(splsw_splx),
	[J_SPLX_NOPROF] =	offsetofsplsw(splsw_splx_noprof),
	[J_SPLRAISE] =		offsetofsplsw(splsw_splraise),
	[J_SPLINTR] =		offsetofsplsw(splsw_splintr),
	[J_SETSOFTINTR] =	offsetofsplsw(splsw_setsoftintr),
	[J_CLRSOFTINTR] =	offsetofsplsw(splsw_clrsoftintr),
};
#endif

void
fixup_splcalls(void)
{
	extern uint32_t _ftext[];
	extern uint32_t _etext[];

#define	addr2offset(x)	((((uint32_t)(uintptr_t)(x)) << 4) >> 6)
#define	stuboffset(x)	addr2offset(x)
	uint32_t splstubs[J_SPLMAX] = {
		[J_SPLHIGH] =		stuboffset(splhigh),
		[J_SPLHIGH_NOPROF] =	stuboffset(splhigh_noprof),
		[J_SPLSCHED] =		stuboffset(splsched),
		[J_SPLVM] =		stuboffset(splvm),
		[J_SPLSOFTSERIAL] =	stuboffset(splsoftserial),
		[J_SPLSOFTNET] =	stuboffset(splsoftnet),
		[J_SPLSOFTBIO] =	stuboffset(splsoftbio),
		[J_SPLSOFTCLOCK] =	stuboffset(splsoftclock),
		[J_SPL0] =		stuboffset(spl0),
		[J_SPLX] =		stuboffset(splx),
		[J_SPLX_NOPROF] =	stuboffset(splx_noprof),
		[J_SPLRAISE] =		stuboffset(splraise),
		[J_SPLINTR] =		stuboffset(splintr),
		[J_SETSOFTINTR] =	stuboffset(_setsoftintr),
		[J_CLRSOFTINTR] =	stuboffset(_clrsoftintr),
	};

#define	realoffset(x)	addr2offset(mips_splsw.splsw_##x)
	uint32_t splreal[J_SPLMAX] = {
		[J_SPLHIGH] =		realoffset(splhigh),
		[J_SPLHIGH_NOPROF] =	realoffset(splhigh_noprof),
		[J_SPLSCHED] =		realoffset(splsched),
		[J_SPLVM] =		realoffset(splvm),
		[J_SPLSOFTSERIAL] =	realoffset(splsoftserial),
		[J_SPLSOFTNET] =	realoffset(splsoftnet),
		[J_SPLSOFTBIO] =	realoffset(splsoftbio),
		[J_SPLSOFTCLOCK] =	realoffset(splsoftclock),
		[J_SPL0] =		realoffset(spl0),
		[J_SPLX] =		realoffset(splx),
		[J_SPLX_NOPROF] =	realoffset(splx_noprof),
		[J_SPLRAISE] =		realoffset(splraise),
		[J_SPLINTR] =		realoffset(splintr),
		[J_SETSOFTINTR] =	realoffset(_setsoftintr),
		[J_CLRSOFTINTR] =	realoffset(_clrsoftintr),
	};

#if 0
	for (size_t i = 0; i < J_SPLMAX; i++) {
		splreal[i] = (((uint32_t *)&mips_splsw)[splreal[i]] << 4) >> 6;
	}
#endif
	mips_fixup_stubs(_ftext, _etext, splstubs, splreal, J_SPLMAX);
}

/* $NetBSD: spl.h,v 1.3 2000/06/08 23:26:10 bjh21 Exp $ */
/*-
 * Copyright (c) 1998 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */
/*
 * spl.h -- Software priority levels, I think.
 */

#ifndef _ARM26_SPL_H
#define _ARM26_SPL_H

/*
 * These are the different SPL states
 *
 * Each state has an interrupt mask associated with it which
 * indicate which interrupts are allowed.
 */

#define IPL_NONE	0
#define IPL_SOFTCLOCK	1
#define IPL_SOFTNET	2
#define IPL_BIO		3
#define IPL_NET		4
#define IPL_TTY		5
#define IPL_IMP		6
#define IPL_AUDIO	7
#define IPL_SERIAL	8
#define IPL_CLOCK	9
#define IPL_STATCLOCK	10
#define IPL_SCHED	11
#define IPL_HIGH	12
#define NIPL		IPL_HIGH + 1

#define splsoftnet()	raisespl(IPL_SOFTNET)
#define splsoft()	splsoftnet()
#define splsoftclock()	raisespl(IPL_SOFTCLOCK)
#define splbio()	raisespl(IPL_BIO)
#define splnet()	raisespl(IPL_NET)
#define spltty()	raisespl(IPL_TTY)
#define splimp()	raisespl(IPL_IMP)
#define	splaudio()	raisespl(IPL_AUDIO)
#define splclock()	raisespl(IPL_CLOCK)
#define splstatclock()	raisespl(IPL_STATCLOCK)
#define splhigh()	raisespl(IPL_HIGH)

#define spl0()			lowerspl(IPL_NONE)
#define spllowersoftclock()	lowerspl(IPL_SOFTCLOCK)
#define splx(s)			lowerspl(s)

#define signotify(p)	setsoftast()

#ifdef _KERNEL
#ifndef ASSEMBLER
extern int raisespl(int);
extern void lowerspl(int);

void setsoftnet(void);
void setsoftast(void);
void setsoftclock(void);
void setsoftintr(u_int intrmask);

extern int current_spl_level; /* XXX tautological name */

void need_resched(void);
void need_proftick(struct proc *);

#endif /* ASSEMBLER */
#endif /* _KERNEL */

#endif /* ! _ARM26_SPL_H */

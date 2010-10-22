/*	$NetBSD: mvsocgppvar.h,v 1.1.4.2 2010/10/22 09:23:12 uebayasi Exp $	*/
/*
 * Copyright (c) 2008, 2008 KIYOHARA Takashi
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MVSOCGPPVAR_H_
#define _MVSOCGPPVAR_H_


static __inline void *
mvsocgpp_intr_establish(int pin, int ipl, int type,
			int (*func)(void *), void *arg)
{

	return intr_establish(gpp_irqbase + pin, ipl, type, func, arg);
}
#define mvsocgpp_intr_disestablish(ih)	intr_disestablish(ih)

int mvsocgpp_pin_read(void *, int);
void mvsocgpp_pin_write(void *, int, int);
void mvsocgpp_pin_ctl(void *, int, int);

#endif	/* _MVSOCGPPVAR_H_ */

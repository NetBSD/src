/* $NetBSD: ppcstart.c,v 1.2 1999/12/30 21:09:56 is Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ignatios Souvatzis.
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

/*
 * Startit for Phase5 PPC boards. 
 */

#include <sys/types.h>
#include "libstubs.h"

#include "kickstart68.c"

void
startit(kp, ksize, entry, fmem, fmemsz, cmemsz,
	    boothowto, esym, cpuid, eclock, amiga_flags, I_flag,
	    bootpartoff, history)

	u_long kp, ksize, entry, fmem, fmemsz, cmemsz,
		boothowto, esym, cpuid, eclock, amiga_flags, I_flag,
		bootpartoff, history;
{
	int i;
	
#define ONESEC
/* #define ONESEC for (i=0; i<1000000; i++) */
	
	ONESEC *(volatile u_int16_t *)0xdff180 = 0x0f0;
	*(volatile u_int8_t *)0xf60000 = 0x10;
	ONESEC *(volatile u_int16_t *)0xdff180 = 0xf80;

	memcpy((caddr_t)0xfff00100, kickstart, kicksize);
	*(volatile u_int32_t *)0xfff000f4 = fmem;
	*(volatile u_int32_t *)0xfff000f8 = kp;
	*(volatile u_int32_t *)0xfff000fc = ksize;
	ONESEC *(volatile u_int16_t *)0xdff180 = 0xf00;
	*(volatile u_int8_t *)0xf60000 = 0x90;
	*(volatile u_int8_t *)0xf60000 = 0x08;
	/* NOTREACHED */
	while (1) {
		*(volatile u_int16_t *)0xdff180 = 0x0f0;
		*(volatile u_int16_t *)0xdff180 = 0x00f;
	}
}

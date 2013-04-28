/*	$NetBSD: epoc32.h,v 1.1 2013/04/28 12:11:26 kiyohara Exp $	*/
/*
 * Copyright (c) 2012 KIYOHARA Takashi
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

#ifndef _EPOC32_H_
#define _EPOC32_H_

#define ARM7XX_INTRREG_VBASE	0xe0000000
#define ARM7XX_INTRREG_BASE	0x80000000
#define ARM7XX_INTRREG_SIZE	0x00001000	/* 4K byte */
#define ARM7XX_FB_VBASE		0xf0000000
#define ARM7XX_FB_BASE		0xc0000000
#define ARM7XX_FB_SIZE		(640 * 240 * 4/*bpp*/ / 8/*bits*/)

struct external_attach_args {
	const char *name;
	bus_space_tag_t iot;
	bus_addr_t addr;
	bus_addr_t addr2;
	int irq;
};

extern int (*soc_find_pending_irqs)(void);
extern void (*soc_initclocks)(void);
extern void (*soc_delay)(unsigned int);

#endif	/* _EPOC32_H_ */

/*
 * Copyright (c) 2007 Danger Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Danger Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#ifndef _ARM_OMAP_HWCLOCK_H_
#define _ARM_OMAP_HWCLOCK_H_

enum hwclock_arch_attr {
	AUTOIDLE,
	SOURCE,
	HWSWREQ,
};

enum hwclock_mclk_src {
	MCLK_SYSTEM_CLOCK,	/* system clock on MCLK, boot default */
	MCLK_APLL48,		/* APLL 48MHz clock on MCLK */
	MCLK_APLL48DIV,		/* APLL 48MHz clock divided down on MCLK */
};

enum hwclock_hwswreq {
	HWREQ,
	SWREQ,
};

#define FLAG_HWREQ		0x0001 /* Use h/w, not s/w, clock request */

struct hwclock_machine {
	unsigned int freq_field;
	unsigned long enable_reg;
	unsigned int enable_field;
	unsigned int swreqenable_bit;
	unsigned int hwreqdisable_bit;
	unsigned int flags;
	unsigned long pm_reg;
	unsigned int pm_field;
	unsigned int src;
};


extern int hwclock_omap1_suspend(void);
extern void hwclock_omap1_resume(void);

#endif /* _ARM_OMAP_HWCLOCK_H_ */

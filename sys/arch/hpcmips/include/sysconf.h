/*	$NetBSD: sysconf.h,v 1.11 2001/09/23 14:32:52 uch Exp $	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
/*
 * Additional reworking by Matthew Jacob for NASA/Ames Research Center.
 * Copyright (c) 1997
 */

/*
 * Copyright (c) 1998 Jonathan Stone.  All rights reserved.
 */

/*
 * Copyright (c) 1999 Shin Takemura.  All rights reserved.
 */

#ifndef	_HPCMIPS_SYSCONF_H_
#define	_HPCMIPS_SYSCONF_H_

#ifdef _KERNEL
/*
 * Platform(VR/TX) Specific Information and Function Hooks.
 */
struct platform_clock;
struct clock_ymdhms;

extern struct platform {
	/*
	 *	cpu_intr	-	interrupt handler
	 *	cpu_idle	-	CPU dependend idle routine.
	 *	cons_init 	-	console initialization
	 *	fb_init         -       frame buffer initialization
	 *      mem_init        -       Count available memory
	 *	reboot		-	reboot or powerdown
	 *	clock		-
	 */
	void	(*cpu_intr)(u_int32_t, u_int32_t, u_int32_t, u_int32_t);
	void	(*cpu_idle)(void);
	void	(*cons_init)(void);
	void	(*fb_init)(caddr_t*);
	void	(*mem_init)(paddr_t);
	void	(*reboot)(int, char *);
	struct platform_clock *clock;
} platform;

struct platform_clock {
	int	hz;
	void	(*init)(struct device *);
	void	(*rtc_get)(struct device *, time_t, struct clock_ymdhms *);
	void	(*rtc_set)(struct device *, struct clock_ymdhms *);
	void	*self;
	int	start;
};

void platform_clock_attach(void *, struct platform_clock *);

#endif /* _KERNEL */
#endif /* !_HPCMIPS_SYSCONF_H_ */

/* $NetBSD: sysconf.h,v 1.3.4.1 1999/11/15 00:39:01 fvdl Exp $ */

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
 * Additional reworking for pmaxes.
 * Since pmax mboard support different CPU daughterboards,
 * and others are mmultiprocessors, rename from cpu_* to sys_*.
 */

#ifndef	_PMAX_SYSCONF_H_
#define	_PMAX_SYSCONF_H_


#ifdef _KERNEL
/*
 * Platform Specific Information and Function Hooks.
 *
 * The tag iobus describes the primary iobus for the platform, primarily
 * to give a hint as to where to start configuring.
 */

struct platform {
	const char	*iobus;		/* Primary iobus name */

	/*
	 * Platform Specific Function Hooks
	 *	bus_reset	-	clear memory error condition
	 *	cons_init	-	console initialization
	 *	device_register -	boot configuration aid
	 *	iointr		-	I/O interrupt handler
	 *	clkread		-	interporate HZ with hi-resolution timer
	 */
	void	(*bus_reset) __P((void));
	void	(*cons_init) __P((void));
	void	(*device_register) __P((struct device *, void *));
	int	(*iointr) __P((unsigned, unsigned, unsigned, unsigned));
	unsigned (*clkread) __P((void));
};

/*
 * An array of functions to initialize the platform structure.
 */
struct sysinit {
	void	(*init) __P((void));
	const char *option;
};

#define	sys_notsupp(st)		{ platform_not_supported, st }
#define	sys_init(fn, opt)	{ fn, opt }

extern struct platform platform;
extern struct sysinit sysinit[];
extern int nsysinit;
extern void platform_not_configured __P((void));
extern void platform_not_supported __P((void));

#endif /* _KERNEL */
#endif /* !_PMAX_SYSCONF_H_ */

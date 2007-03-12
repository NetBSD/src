/*	$NetBSD: sysconf.h,v 1.3.6.1 2007/03/12 05:49:24 rmind Exp $	*/

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

#ifndef	_MIPSCO_SYSCONF_H_
#define	_MIPSCO_SYSCONF_H_

#ifdef _KERNEL

/*
 * Platform Specific Information and Function Hooks.
 */

struct clock_ymdhms;

struct platform {
	const char	*iobus;		/* Primary iobus name */

	/*
	 * Platform Specific Function Hooks
	 *	cons_init	-	console initialization
	 *	iointr		-	I/O interrupt handler
	 *	memsize		-	Size external memory
	 *	read_todr	-	Read TOD registers
	 *	write_todr	-	Write TOD registers
	 *	clkinit		-	Initialize clocks
	 */
	void	(*cons_init) __P((void));
	void	(*iointr) __P((unsigned, unsigned, unsigned, unsigned));
	int	(*memsize) __P((void *));
	void	(*intr_establish) __P((int, int (*)__P((void *)), void *)); 
	void	(*clkinit) __P((void));
};

extern struct platform platform;

#endif /* _KERNEL */

#endif	/* !_MIPSCO_SYSCONF_H_ */

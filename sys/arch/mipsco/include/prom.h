/*	$NetBSD: prom.h,v 1.1 2000/08/19 12:13:47 wdk Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wayne Knowles
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
 * Entry points into PROM firmware functions for MIPS machines
 */

#ifndef _MIPSCO_FIRMWARE_H
#define _MIPSCO_FIRMWARE_H

struct mips_prom {

	/*
	 * transferred to on hardware reset, configures MIPS boards,
	 * runs diags, check for appropriate auto boot action in
	 * "bootmode" environment variable and performs that action.
	 */
	void	(*prom_reset)	__P((void)) __attribute__((__noreturn__));

	/*
	 * called to utilize prom to boot new image.  After the booted
	 * program returns control can either be returned to the
	 * original caller of the exec routine or to the prom monitor.
	 * (to return to the original caller, the new program must
	 * not destroy any text, data, or stack of the parent.  the
	 * new programs stack continues on the parents stack.
	 */
	int	(*prom_exec)	__P((void));

	/* re-enter the prom command parser, do not reset prom state	*/
	void	(*prom_restart) __P((void)) __attribute__((__noreturn__));

	/* reinitialize prom state and re-enter the prom command parser */
	void	(*prom_reinit)	__P((void)) __attribute__((__noreturn__));

	/* reboot machine using current bootmode setting.  No diags */
	void	(*prom_reboot)	__P((void)) __attribute__((__noreturn__));

	/* perform an autoboot sequence, no configuration or diags run */
	void	(*prom_autoboot) __P((void)) __attribute__((__noreturn__));

	/*
	 * these routines access prom "saio" routines, and may be used
	 * by standalone programs that would like to use prom io
	 */
	int	(*prom_open)		__P((char *, int, ...));
	int	(*prom_read)		__P((int, void *, int));
	int	(*prom_write)		__P((int, void *, int));
	int	(*prom_ioctl)		__P((int));
	int	(*prom_close)		__P((int));
	int	(*prom_getchar)		__P((void));
	int	(*prom_putchar)		__P((int c));
	void	(*prom_showchar)	__P((int c));
	char * 	(*prom_gets)		__P((char *s));
	void	(*prom_puts)		__P((const char *));
	int	(*prom_printf)		__P((const char *, ...));

	/* prom protocol entry points */
	void	(*prom_initproto)	__P((void)); /* ??? */
	void	(*prom_protoenable)	__P((void)); /* ??? */
	void	(*prom_protodisable)	__P((void)); /* ??? */
	void	(*prom_getpkt)		__P((void)); /* ??? */
	void	(*prom_putpkt)		__P((void)); /* ??? */

	/*
	 * read-modify-write routine use special cpu board circuitry to
	 * accomplish vme bus r-m-w cycles.
	 */
	void	(*prom_orw_rmw)		__P((void));
	void	(*prom_orh_rmw)		__P((void));
	void	(*prom_orb_rmw)		__P((void));
	void	(*prom_andw_rmw)	__P((void));
	void	(*prom_andh_rmw)	__P((void));
	void	(*prom_andb_rmw)	__P((void));

	/*
	 * cache control entry points
	 * flushcache is called without arguments and invalidates entire
	 *      contents of both i and d caches
	 * clearcache is called with a base address and length (where
	 *      address is either K0, K1, or physical) and clears both
	 *      i and d cache for entries that alias to specified address
	 *      range.
	 */
	void	(*prom_flushcache)	__P((void));
	void	(*prom_clearcache)	__P((void));

	/*
	 * Libc compatible functions
	 */
	void	(*prom_setjmp)		__P((void));
	void	(*prom_longjmp)		__P((void));
	void	(*prom_bevutlb)		__P((void));
	char *	(*prom_getenv)		__P((char *name));
	int	(*prom_setenv)		__P((char *name, char *val));
	int	(*prom_atob)		__P((char *s));
	int	(*prom_strcmp)		__P((char *s1, char *s2));
	int	(*prom_strlen)		__P((char *s));
	char *	(*prom_strcpy)		__P((char *s1, char *s2));
	char *	(*prom_strcat)		__P((char *s1, char *s2));

	/*
	 * command parser entry points
	 */
	void	(*prom_parser)		__P((void)); /* ??? */
	void	(*prom_range)		__P((void)); /* ??? */
	void	(*prom_argvize)		__P((void)); /* ??? */
	void	(*prom_help)		__P((void));

	/*
	 * prom commands
	 */
	void	(*prom_dumpcmd)		__P((void));
	void	(*prom_setenvcmd)	__P((void));
	void	(*prom_unsetenvcmd)	__P((void));
	void	(*prom_bevexcept)	__P((void));
	void	(*prom_enablecmd)	__P((void));
	void	(*prom_disablecmd)	__P((void));

	/*
	 * clear existing fault handlers
	 * used by clients that link to prom on situations where client has
	 * interrupted out of prom code and wish to reenter without being
	 * tripped up by any pending prom timers set earlier.
	 */
	void	(*prom_clearnofault)	__P((void));

	void	(*prom_notimpl)		__P((void));

	/*
	 * PROM_NVGET, PROM_NVSET will get/set information in the NVRAM.
	 * Both of these routines take indexes as opposed to addresses
	 * to guarantee portability between various platforms
	 */
	void	(*prom_nvget)		__P((void));
	void	(*prom_nv_set)		__P((void));
};

extern struct mips_prom *callv;

/*
 * Macro to help call a prom function
 */
#define MIPS_PROM(func)		(callv->prom_##func)

/*
 * Return the address for a given prom function number */
#define	PROM_ENTRY(x)		(0xbfc00000+((x)*8))

#endif /* ! _MIPSCO_FIRMWARE_H */

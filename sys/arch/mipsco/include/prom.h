/*	$NetBSD: prom.h,v 1.4.2.3 2000/12/08 09:28:24 bouyer Exp $	*/

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

#ifndef _MIPSCO_PROM_H
#define _MIPSCO_PROM_H

#ifndef _LOCORE
#include <sys/types.h>
#include <sys/cdefs.h>

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
	int	(*prom_ioctl)		__P((int, long, ...));
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
	void	(*prom_clearcache)	__P((void *, size_t));

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
	int	(*prom_nvget)		__P((int));
	void	(*prom_nvset)		__P((void));
};

extern struct mips_prom *callv;

extern void prom_init __P((void));

#endif	/* _LOCORE */

/*
 * Macro to help call a prom function
 */
#define MIPS_PROM(func)		(callv->prom_##func)

/*
 * Return the address for a given prom function number
 */
#define	MIPS_PROM_ENTRY(x)	(0xbfc00000+((x)*8))

/*
 * MIPS PROM firmware functions:
 *
 *	MIPS_PROM_RESET		Run diags, check bootmode, reinit.
 *	MIPS_PROM_EXEC		Load new program image.
 *	MIPS_PROM_RESTART	Re-enter monitor command loop.
 *	MIPS_PROM_REINIT	Re-init monitor, then cmd loop.
 *	MIPS_PROM_REBOOT	Check bootmode, no config.
 *	MIPS_PROM_AUTOBOOT	Autoboot the system.
 *
 * The following routines access PROM saio routines and may be used by
 * standalone programs that would like to use PROM I/O:
 *
 *	MIPS_PROM_OPEN		Open a file.
 *	MIPS_PROM_READ		Read from a file.
 *	MIPS_PROM_WRITE		Write to a file.
 *	MIPS_PROM_IOCTL		Ioctl on a file.
 *	MIPS_PROM_CLOSE		Close a file.
 *	MIPS_PROM_GETCHAR	Read character from console.
 *	MIPS_PROM_PUTCHAR	Put character on console.
 *	MIPS_PROM_SHOWCHAR	Show a char visibly.
 *	MIPS_PROM_GETS		gets with editing.
 *	MIPS_PROM_PUTS		Put string to console.
 *	MIPS_PROM_PRINTF	Kernel style printf to console.
 *
 *  PROM protocol entry points:
 *
 *	MIPS_PROM_INITPROTO	Initialize protocol.
 *	MIPS_PROM_PROTOENABLE	Enable protocol mode.
 *	MIPS_PROM_PROTODISABLE	Disable protocol mode.
 *	MIPS_PROM_GETPKT	Get protocol packet.
 *	MIPS_PROM_PUTPKT	Put protocol packet.
 *
 *   Atomic Read-Modify-Write functions
 *
 *	MIPS_PROM_RMW_OR32	r-m-w OR word.
 *	MIPS_PROM_RMW_OR16	r-m-w OR halfword.
 *	MIPS_PROM_RMW_OR8	r-m-w OR or byte.
 *	MIPS_PROM_RMW_AND32	r-m-w AND word.
 *	MIPS_PROM_RMW_AND16	r-m-w AND halfword.
 *	MIPS_PROM_RMW_AND8	r-m-w AND byte.
 *
 *	MIPS_PROM_FLUSHCACHE	Flush entire cache ().
 *	MIPS_PROM_CLEARCACHE	Clear I & D cache in range (addr, len).
 *	MIPS_PROM_SETJMP	Save registers in a buffer.
 *	MIPS_PROM_LONGJMP	Get register back from buffer.
 *	MIPS_PROM_BEVUTLB	utlbmiss boot exception vector
 *	MIPS_PROM_GETENV	Gets a string from system environment.
 *	MIPS_PROM_SETENV	Sets a string in system environment.
 *	MIPS_PROM_ATOB		Converts ascii string to number.
 *	MIPS_PROM_STRCMP	Compares strings (strcmp).
 *	MIPS_PROM_STRLEN	Length of string (strlen).
 *	MIPS_PROM_STRCPY	Copies string (strcpy).
 *	MIPS_PROM_STRCAT	Appends string (strcat).
 *	MIPS_PROM_PARSER	Command parser.
 *	MIPS_PROM_RANGE		Address range parser
 *	MIPS_PROM_ARGVIZE	Parses string to argc,argv.
 *	MIPS_PROM_HELP		Help on prom commands.
 *	MIPS_PROM_DUMP		Dump memory command.
 *	MIPS_PROM_SETENVCMD	Setenv command.
 *	MIPS_PROM_UNSETENVCMD	Unsetenv command.
 *	MIPS_PROM_PRINTENV	Print environment command.
 *	MIPS_PROM_BEVEXCEPT	General boot exception vector 
 *	MIPS_PROM_ENABLE	Performs prom enable command.
 *	MIPS_PROM_DISABLE	Performs prom disable command.
 *	MIPS_PROM_CLEARNOFAULT	Clear existing fault handlers.
 *	MIPS_PROM_NOTIMPL	Guaranteed to be not implemented
 *	MIPS_PROM_NVGET		Read bytes from NVRAM
 *	MIPS_PROM_NVSET		Write bytes to NVRAM
 */

#define	MIPS_PROM_RESET		MIPS_PROM_ENTRY(0)
#define	MIPS_PROM_EXEC		MIPS_PROM_ENTRY(1)
#define	MIPS_PROM_RESTART	MIPS_PROM_ENTRY(2)
#define	MIPS_PROM_REINIT	MIPS_PROM_ENTRY(3)
#define	MIPS_PROM_REBOOT	MIPS_PROM_ENTRY(4)
#define	MIPS_PROM_AUTOBOOT	MIPS_PROM_ENTRY(5)
#define	MIPS_PROM_OPEN		MIPS_PROM_ENTRY(6)
#define	MIPS_PROM_READ		MIPS_PROM_ENTRY(7)
#define	MIPS_PROM_WRITE		MIPS_PROM_ENTRY(8)
#define	MIPS_PROM_IOCTL		MIPS_PROM_ENTRY(9)
#define	MIPS_PROM_CLOSE		MIPS_PROM_ENTRY(10)
#define	MIPS_PROM_GETCHAR	MIPS_PROM_ENTRY(11)
#define	MIPS_PROM_PUTCHAR	MIPS_PROM_ENTRY(12)
#define	MIPS_PROM_SHOWCHAR	MIPS_PROM_ENTRY(13)
#define	MIPS_PROM_GETS		MIPS_PROM_ENTRY(14)
#define	MIPS_PROM_PUTS		MIPS_PROM_ENTRY(15)
#define	MIPS_PROM_PRINTF	MIPS_PROM_ENTRY(16)
#define	MIPS_PROM_INITPROTO	MIPS_PROM_ENTRY(17)
#define	MIPS_PROM_PROTOENABLE	MIPS_PROM_ENTRY(18)
#define	MIPS_PROM_PROTODISABLE	MIPS_PROM_ENTRY(19)
#define	MIPS_PROM_GETPKT	MIPS_PROM_ENTRY(20)
#define	MIPS_PROM_PUTPKT	MIPS_PROM_ENTRY(21)
#define	MIPS_PROM_ORW_RMW	MIPS_PROM_ENTRY(22)
#define	MIPS_PROM_ORH_RMW	MIPS_PROM_ENTRY(23)
#define	MIPS_PROM_ORB_RMW	MIPS_PROM_ENTRY(24)
#define	MIPS_PROM_ANDW_RMW	MIPS_PROM_ENTRY(25)
#define	MIPS_PROM_ANDH_RMW	MIPS_PROM_ENTRY(26)
#define	MIPS_PROM_ANDB_RMW	MIPS_PROM_ENTRY(27)
#define	MIPS_PROM_FLUSHCACHE	MIPS_PROM_ENTRY(28)
#define	MIPS_PROM_CLEARCACHE	MIPS_PROM_ENTRY(29)
#define	MIPS_PROM_SETJMP	MIPS_PROM_ENTRY(30)
#define	MIPS_PROM_LONGJMP	MIPS_PROM_ENTRY(31)
#define	MIPS_PROM_BEVUTLB	MIPS_PROM_ENTRY(32)
#define	MIPS_PROM_GETENV	MIPS_PROM_ENTRY(33)
#define	MIPS_PROM_SETENV	MIPS_PROM_ENTRY(34)
#define	MIPS_PROM_ATOB		MIPS_PROM_ENTRY(35)
#define	MIPS_PROM_STRCMP	MIPS_PROM_ENTRY(36)
#define	MIPS_PROM_STRLEN	MIPS_PROM_ENTRY(37)
#define	MIPS_PROM_STRCPY	MIPS_PROM_ENTRY(38)
#define	MIPS_PROM_STRCAT	MIPS_PROM_ENTRY(39)
#define	MIPS_PROM_PARSER	MIPS_PROM_ENTRY(40)
#define	MIPS_PROM_RANGE		MIPS_PROM_ENTRY(41)
#define	MIPS_PROM_ARGVIZE	MIPS_PROM_ENTRY(42)
#define	MIPS_PROM_HELP		MIPS_PROM_ENTRY(43)
#define	MIPS_PROM_DUMP		MIPS_PROM_ENTRY(44)
#define	MIPS_PROM_SETENVCMD	MIPS_PROM_ENTRY(45)
#define	MIPS_PROM_UNSETENVCMD	MIPS_PROM_ENTRY(46)
#define	MIPS_PROM_PRINTENV	MIPS_PROM_ENTRY(47)
#define	MIPS_PROM_BEVEXCEPT	MIPS_PROM_ENTRY(48)
#define	MIPS_PROM_ENABLE	MIPS_PROM_ENTRY(49)
#define	MIPS_PROM_DISABLE	MIPS_PROM_ENTRY(50)
#define	MIPS_PROM_CLEARNOFAULT	MIPS_PROM_ENTRY(51)
#define	MIPS_PROM_NOTIMPLEMENT	MIPS_PROM_ENTRY(52)
#define MIPS_PROM_NVGET		MIPS_PROM_ENTRY(53)
#define MIPS_PROM_NVSET		MIPS_PROM_ENTRY(54)

#ifdef _KERNEL
extern void prom_init (void);
extern int prom_getconsole (void);
#endif

#endif /* ! _MIPSCO_PROM_H */

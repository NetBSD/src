/*	$NetBSD: dec_prom.h,v 1.23.32.1 2016/07/09 20:24:55 skrll Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)dec_prom.h	8.1 (Berkeley) 6/10/93
 *
 * machMon.h --
 *
 *	Structures, constants and defines for access to the pmax prom.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/machMon.h,
 *	v 9.3 90/02/20 14:34:07 shirriff Exp  SPRITE (Berkeley)
 */

#ifndef _PMAX_DEC_PROM_H_
#define _PMAX_DEC_PROM_H_

/*
 * This file was created based on information from the document
 * "TURBOchannel Firmware Specification" (EK-TCAAD-FS-003)
 * by Digital Equipment Corporation.
 */

#ifndef _LOCORE
#include <sys/types.h>
#include <sys/cdefs.h>
#include <machine/int_types.h>

/*
 * Programs loaded by the new PROMs pass the following arguments:
 *	a0	argc
 *	a1	argv
 *	a2	DEC_PROM_MAGIC
 *	a3	The callback vector defined below
 */

#define DEC_PROM_MAGIC	0x30464354

typedef struct memmap {
	int	pagesize;	/* system page size */
	u_char	bitmap[15360];	/* bit for each page indicating safe to use */
} memmap;

typedef struct {
	int	revision;	/* hardware revision level */
	int	clk_period;	/* clock period in nano seconds */
	int	slot_size;	/* slot size in magabytes */
	int	io_timeout;	/* I/O timeout in cycles */
	int	dma_range;	/* DMA address range in megabytes */
	int	max_dma_burst;	/* maximum DMA burst length */
	int	parity;		/* true if system module supports T.C. parity */
	int	reserved[4];
} tcinfo;

typedef int jmp_buf[12];
typedef void (*psig_t)(int);

struct callback {
	void	*(*_memcpy)(void *, void *, int);		/* 00 */
	void	*(*_memset)(void *, int, int);			/* 04 */
	char	*(*_strcat)(char *, char *);			/* 08 */
	int	(*_strcmp)(char *, char *);			/* 0c */
	char	*(*_strcpy)(char *, char *);			/* 10 */
	int	(*_strlen)(char *);				/* 14 */
	char	*(*_strncat)(char *, char *, int);		/* 18 */
	char	*(*_strncpy)(char *, char *, int);		/* 1c */
	int	(*_strncmp)(char *, char *, int);		/* 20 */
	int	(*_getchar)(void);				/* 24 */
	char	*(*_unsafe_gets)(char *);			/* 28 */
	int	(*_puts)(char *);				/* 2c */
	int	(*_printf)(const char *, ...);			/* 30 */
	int	(*_sprintf)(char *, char *, ...);		/* 34 */
	int	(*_io_poll)(void);				/* 38 */
	long	(*_strtol)(char *, char **, int);		/* 3c */
	psig_t	(*_signal)(int, psig_t);			/* 40 */
	int	(*_raise)(int);					/* 44 */
	long	(*_time)(long *);				/* 48 */
	int	(*_setjmp)(jmp_buf);				/* 4c */
	void	(*_longjmp)(jmp_buf, int);			/* 50 */
	int	(*_bootinit)(char *);				/* 54 */
	int	(*_bootread)(int, void *, int);			/* 58 */
	int	(*_bootwrite)(int, void *, int);		/* 5c */
	int	(*_setenv)(char *, char *);			/* 60 */
	char	*(*_getenv)(const char *);			/* 64 */
	int	(*_unsetenv)(char *);				/* 68 */
	u_long	(*_slot_address)(int);				/* 6c */
	void	(*_wbflush)(void);				/* 70 */
	void	(*_msdelay)(int);				/* 74 */
	void	(*_leds)(int);					/* 78 */
	void	(*_clear_cache)(char *, int);			/* 7c */
	int	(*_getsysid)(void);				/* 80 */
	int	(*_getbitmap)(memmap *);			/* 84 */
	int	(*_disableintr)(int);				/* 88 */
	int	(*_enableintr)(int);				/* 8c */
	int	(*_testintr)(int);				/* 90 */
	void	*_reserved_data;				/* 94 */
	int	(*_console_init)(void);				/* 98 */
	void	(*_halt)(int *, int);				/* 9c */
	void	(*_showfault)(void);				/* a0 */
	tcinfo	*(*_gettcinfo)(void);	 /* XXX bogus proto */	/* a4 */
	int	(*_execute_cmd)(char *);			/* a8 */
	void	(*_rex)(char);					/* ac */
	/* b0 to d4 reserved */
};

extern const struct callback *callv;
#ifdef _LP64
extern struct callback callvec;
#else
extern const struct callback callvec;
#endif

#ifdef _KERNEL
intptr_t promcall(void *, ...);
#endif

#if defined(_STANDALONE) && !defined(_NO_PROM_DEFINES)
#define memcpy (*callv -> _memcpy)
#define memset (*callv -> _memset)
#define strcat (*callv -> _strcat)
#define strcmp (*callv -> _strcmp)
#define strcpy (*callv -> _strcpy)
#define strlen (*callv -> _strlen)
#define strncat (*callv -> _strncat)
#define strncpy (*callv -> _strncpy)
#define strncmp (*callv -> _strncmp)
#define getchar (*callv -> _getchar)
#define unsafe_gets (*callv -> _unsafe_gets)
#define puts (*callv -> _puts)
#define printf (*callv -> _printf)
#define sprintf (*callv -> _sprintf)
#define io_poll (*callv -> _io_poll)
#define strtol (*callv -> _strtol)
#define raise (*callv -> _raise)
#define time (*callv -> _time)
#define setjmp (*callv -> _setjmp)
#define longjmp (*callv -> _longjmp)
#define bootinit (*callv -> _bootinit)
#define bootread (*callv -> _bootread)
#define bootwrite (*callv -> _bootwrite)
#define setenv (*callv -> _setenv)
#define getenv (*callv -> _getenv)
#define unsetenv (*callv -> _unsetenv)
#define wbflush (*callv -> _wbflush)
#define msdelay (*callv -> _msdelay)
#define leds (*callv -> _leds)
#define clear_cache (*callv -> _clear_cache)
#define getsysid (*callv -> _getsysid)
#define getbitmap (*callv -> _getbitmap)
#define disableintr (*callv -> _disableintr)
#define enableintr (*callv -> _enableintr)
#define testintr (*callv -> _testintr)
#define console_init (*callv -> _console_init)
#define halt (*callv -> _halt)
#define showfault (*callv -> _showfault)
#define gettcinfo (*callv -> _gettcinfo)
#define execute_cmd (*callv -> _execute_cmd)
#define rex (*callv -> _rex)

#define bzero(dst, len) memset(dst, 0, len)
/* XXX make sure that no calls to bcopy overlap! */
#define bcopy(src, dst, len) memcpy(dst, src, len)
#endif

/*
 * The prom routines use the following structure to hold strings.
 */
typedef struct {
	char	*argPtr[16];	/* Pointers to the strings. */
	char	strings[256];	/* Buffer for the strings. */
	char	*end;		/* Pointer to end of used buf. */
	int 	num;		/* Number of strings used. */
} MachStringTable;

#endif /* _LOCORE */

/*
 * The prom has a jump table at the beginning of it to get to its
 * functions.
 */
#define DEC_PROM_JUMP_TABLE_ADDR	0xBFC00000

/*
 * Each entry in the jump table is 8 bytes - 4 for the jump and 4 for a nop.
 */
#define DEC_PROM_FUNC_ADDR(funcNum)	(DEC_PROM_JUMP_TABLE_ADDR+((funcNum)*8))

/*
 * The functions:
 *
 *	DEC_PROM_RESET		Run diags, check bootmode, reinit.
 *	DEC_PROM_EXEC		Load new program image.
 *	DEC_PROM_RESTART	Re-enter monitor command loop.
 *	DEC_PROM_REINIT		Re-init monitor, then cmd loop.
 *	DEC_PROM_REBOOT		Check bootmode, no config.
 *	DEC_PROM_AUTOBOOT	Autoboot the system.
 *
 * The following routines access PROM saio routines and may be used by
 * standalone programs that would like to use PROM I/O:
 *
 *	DEC_PROM_OPEN		Open a file.
 *	DEC_PROM_READ		Read from a file.
 *	DEC_PROM_WRITE		Write to a file.
 *	DEC_PROM_IOCTL		Iocontrol on a file.
 *	DEC_PROM_CLOSE		Close a file.
 *	DEC_PROM_LSEEK		Seek on a file.
 *	DEC_PROM_GETCHAR	Get character from console.
 *	DEC_PROM_PUTCHAR	Put character on console.
 *	DEC_PROM_SHOWCHAR	Show a char visibly.
 *	DEC_PROM_GETS		gets with editing.
 *	DEC_PROM_PUTS		Put string to console.
 *	DEC_PROM_PRINTF		Kernel style printf to console.
 *
 *  PROM protocol entry points:
 *
 *	DEC_PROM_INITPROTO	Initialize protocol.
 *	DEC_PROM_PROTOENABLE	Enable protocol mode.
 *	DEC_PROM_PROTODISABLE	Disable protocol mode.
 *	DEC_PROM_GETPKT		Get protocol packet.
 *	DEC_PROM_PUTPKT		Put protocol packet.
 *
 * The following are other prom routines:
 *	DEC_PROM_FLUSHCACHE	Flush entire cache ().
 *	DEC_PROM_CLEARCACHE	Clear I & D cache in range (addr, len).
 *	DEC_PROM_SAVEREGS	Save registers in a buffer.
 *	DEC_PROM_LOADREGS	Get register back from buffer.
 *	DEC_PROM_JUMPS8		Jump to address in s8.
 *	DEC_PROM_GETENV2	Gets a string from system environment.
 *	DEC_PROM_SETENV2	Sets a string in system environment.
 *	DEC_PROM_ATONUM		Converts ascii string to number.
 *	DEC_PROM_STRCMP		Compares strings (strcmp).
 *	DEC_PROM_STRLEN		Length of string (strlen).
 *	DEC_PROM_STRCPY		Copies string (strcpy).
 *	DEC_PROM_STRCAT		Appends string (strcat).
 *	DEC_PROM_GETCMD		Gets a command.
 *	DEC_PROM_GETNUMS	Gets numbers.
 *	DEC_PROM_ARGPARSE	Parses string to argc,argv.
 *	DEC_PROM_HELP		Help on prom commands.
 *	DEC_PROM_DUMP		Dumps memory.
 *	DEC_PROM_SETENV		Sets a string in system environment.
 *	DEC_PROM_UNSETENV	Unsets a string in system environment
 *	DEC_PROM_PRINTENV	Prints system environment
 *	DEC_PROM_JUMP2S8	Jumps to s8
 *	DEC_PROM_ENABLE		Performs prom enable command.
 *	DEC_PROM_DISABLE	Performs prom disable command.
 *	DEC_PROM_ZEROB		Zeros a system buffer.
 *	DEC_PROM_HALT		Handler for halt interrupt.
 *	DEC_PROM_STARTCVAX	58xx VAX Diagnostic Supervisor support.
 */
#define DEC_PROM_RESET		DEC_PROM_FUNC_ADDR(0)
#define DEC_PROM_EXEC		DEC_PROM_FUNC_ADDR(1)
#define DEC_PROM_RESTART	DEC_PROM_FUNC_ADDR(2)
#define DEC_PROM_REINIT		DEC_PROM_FUNC_ADDR(3)
#define DEC_PROM_REBOOT		DEC_PROM_FUNC_ADDR(4)
#define DEC_PROM_AUTOBOOT	DEC_PROM_FUNC_ADDR(5)
#define DEC_PROM_OPEN		DEC_PROM_FUNC_ADDR(6)
#define DEC_PROM_READ		DEC_PROM_FUNC_ADDR(7)
#define DEC_PROM_WRITE		DEC_PROM_FUNC_ADDR(8)
#define DEC_PROM_IOCTL		DEC_PROM_FUNC_ADDR(9)
#define DEC_PROM_CLOSE		DEC_PROM_FUNC_ADDR(10)
#define DEC_PROM_LSEEK		DEC_PROM_FUNC_ADDR(11)
#define DEC_PROM_GETCHAR	DEC_PROM_FUNC_ADDR(12)
#define DEC_PROM_PUTCHAR	DEC_PROM_FUNC_ADDR(13)
#define DEC_PROM_SHOWCHAR	DEC_PROM_FUNC_ADDR(14)
#define DEC_PROM_GETS		DEC_PROM_FUNC_ADDR(15)
#define DEC_PROM_PUTS		DEC_PROM_FUNC_ADDR(16)
#define DEC_PROM_PRINTF		DEC_PROM_FUNC_ADDR(17)
#define DEC_PROM_INITPROTO	DEC_PROM_FUNC_ADDR(18)
#define DEC_PROM_PROTOENABLE	DEC_PROM_FUNC_ADDR(19)
#define DEC_PROM_PROTODISABLE	DEC_PROM_FUNC_ADDR(20)
#define DEC_PROM_GETPKT		DEC_PROM_FUNC_ADDR(21)
#define DEC_PROM_PUTPKT		DEC_PROM_FUNC_ADDR(22)
#define DEC_PROM_FLUSHCACHE	DEC_PROM_FUNC_ADDR(28)
#define DEC_PROM_CLEARCACHE	DEC_PROM_FUNC_ADDR(29)
#define DEC_PROM_SAVEREGS	DEC_PROM_FUNC_ADDR(30)
#define DEC_PROM_LOADREGS	DEC_PROM_FUNC_ADDR(31)
#define DEC_PROM_JUMPS8		DEC_PROM_FUNC_ADDR(32)
#define DEC_PROM_GETENV2	DEC_PROM_FUNC_ADDR(33)
#define DEC_PROM_SETENV2	DEC_PROM_FUNC_ADDR(34)
#define DEC_PROM_ATONUM		DEC_PROM_FUNC_ADDR(35)
#define DEC_PROM_STRCMP		DEC_PROM_FUNC_ADDR(36)
#define DEC_PROM_STRLEN		DEC_PROM_FUNC_ADDR(37)
#define DEC_PROM_STRCPY		DEC_PROM_FUNC_ADDR(38)
#define DEC_PROM_STRCAT		DEC_PROM_FUNC_ADDR(39)
#define DEC_PROM_GETCMD		DEC_PROM_FUNC_ADDR(40)
#define DEC_PROM_GETNUMS	DEC_PROM_FUNC_ADDR(41)
#define DEC_PROM_ARGPARSE	DEC_PROM_FUNC_ADDR(42)
#define DEC_PROM_HELP		DEC_PROM_FUNC_ADDR(43)
#define DEC_PROM_DUMP		DEC_PROM_FUNC_ADDR(44)
#define DEC_PROM_SETENV		DEC_PROM_FUNC_ADDR(45)
#define DEC_PROM_UNSETENV	DEC_PROM_FUNC_ADDR(46)
#define DEC_PROM_PRINTENV	DEC_PROM_FUNC_ADDR(47)
#define DEC_PROM_JUMP2S8	DEC_PROM_FUNC_ADDR(48)
#define DEC_PROM_ENABLE		DEC_PROM_FUNC_ADDR(49)
#define DEC_PROM_DISABLE	DEC_PROM_FUNC_ADDR(50)
#define DEC_PROM_ZEROB		DEC_PROM_FUNC_ADDR(51)
#define DEC_PROM_HALT		DEC_PROM_FUNC_ADDR(54)
#define DEC_PROM_STARTCVAX	DEC_PROM_FUNC_ADDR(97)

/*
 * The nonvolatile ram has a flag to indicate it is usable.
 */
#define MACH_USE_NON_VOLATILE 	((char *)0xbd0000c0)
#define MACH_NON_VOLATILE_FLAG	0x02

#define DEC_REX_MAGIC		0x30464354	/* REX Magic number */

#endif	/* !_PMAX_DEC_PROM_H_ */

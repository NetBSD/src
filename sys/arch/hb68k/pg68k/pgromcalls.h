/*	$NetBSD: pgromcalls.h,v 1.1 2026/07/19 01:48:24 thorpej Exp $	*/

/*
 * Copyright (c) 2026 Jason R. Thorpe.
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */     

#ifndef _PG68K_PGROMCALLS_H_
#define	_PG68K_PGROMCALLS_H_

struct pgromcalls {
	int	rv1_version;	/* version of the ROM call vector */

	/*
	 * System boot lifecycle.
	 *
	 * void (*reboot)(void);
	 *	Reboot the system, auto-booting the system, if possible.
	 *
	 * void (*halt)(void);
	 *	Halt the system, returning to the firmware command
	 *	prompt.
	 *
	 * void (*poweroff)(void);
	 *	Power-off the system, if possible.  If power-off is
	 *	not supported or unsuccessful, this is the equivalent
	 *	of (*halt)().
	 */
	void	(*rv1_reboot)(void);
	void	(*rv1_halt)(void);
	void	(*rv1_poweroff)(void);

	/*
	 * Console I/O.
	 *
	 * int (*cnpollc)(void);
	 *	Poll for a character from the console.  If no character
	 *	is available, -1 is returned.
	 *
	 * void (*cnputc)(int ch);
	 *	Output a character to the console.  Newlines are "cooked",
	 *	i.e. a '\n' sent to (*cnputc)() results in "\r\n" being
	 *	sent to the terminal.
	 */
	int	(*rv1_cnpollc)(void);
	void	(*rv1_cnputc)(int);

	/* 
	 * Diagnostic display.
	 *
	 * First argument is the upper digit, and second argument is lower
	 * digit.  0 - 15, '0' - '9', 'A' - 'F', 'a' - 'f', and ' ' are all
	 * valid arguments.  Any unknown value will also blank that digit.
	 */
	void	(*rv1_diag)(unsigned char, unsigned char);
};

#ifdef _KERNEL
void	pgromcall_init(void);
void	pgromcall_reboot(void);
void	pgromcall_halt(void);
void	pgromcall_poweroff(void);

void	pgromcall_diag(uint8_t, uint8_t);
#endif /* _KERNEL */

#endif /* _PG68K_PGROMCALLS_H_ */

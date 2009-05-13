/*	$NetBSD: btattach.h,v 1.1.10.1 2009/05/13 19:20:19 jym Exp $	*/

/*-
 * Copyright (c) 2008 Iain Hibbert
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <stdbool.h>

typedef void (devinit_t)(int, unsigned int);

struct devtype {
	const char *	name;	/* short name */
	const char *	line;	/* line discipline */
	const char *	descr;	/* long description */
	devinit_t *	init;	/* init function */
	tcflag_t	cflag;	/* default cflags */
	unsigned int	speed;	/* default baudrate */
};

devinit_t init_bcm2035;
devinit_t init_bgb2xx;
devinit_t init_csr;
devinit_t init_digi;
devinit_t init_ericsson;
devinit_t init_st;
devinit_t init_stlc2500;
devinit_t init_swave;

void uart_send_cmd(int, uint16_t, void *, size_t);
size_t uart_recv_ev(int, uint8_t, void *, size_t);
size_t uart_recv_cc(int, uint16_t, void *, size_t);

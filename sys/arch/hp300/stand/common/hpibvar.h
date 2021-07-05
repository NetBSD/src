/*	$NetBSD: hpibvar.h,v 1.6 2021/07/05 14:51:23 tsutsui Exp $	*/

/*
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)hpibvar.h	8.1 (Berkeley) 6/10/93
 */

#include <hp300/dev/hpibvar.h>

struct	hpib_softc {
	char	sc_alive;
	char	sc_type;
	int	sc_ba;
	char	*sc_addr;
};

extern struct hpib_softc hpib_softc[];
extern int internalhpib;

/* hpib.c */
void hpibinit(void);
int hpibalive(int);
int hpibid(int, int);
int hpibsend(int, int, int, uint8_t *, int);
int hpibrecv(int, int, int, uint8_t *, int);
int hpibswait(int, int);
void hpibgo(int, int, int, uint8_t *, int, int);

/* fhpib.c */
int fhpibinit(int);
void fhpibreset(int);
int fhpibsend(int, int, int, uint8_t *, int);
int fhpibrecv(int, int, int, uint8_t *, int);
int fhpibppoll(int);

/* nhpib.c */
int nhpibinit(int);
void nhpibreset(int);
int nhpibsend(int, int, int, uint8_t *, int);
int nhpibrecv(int, int, int, uint8_t *, int);
int nhpibppoll(int);


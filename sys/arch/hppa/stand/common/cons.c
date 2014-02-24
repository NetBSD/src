/*	$NetBSD: cons.c,v 1.1 2014/02/24 07:23:43 skrll Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah Hdr: cons.c 1.7 92/02/28
 *
 *	@(#)cons.c	8.1 (Berkeley) 6/10/93
 */

#include "libsa.h"
#include <sys/param.h>
#include <dev/cons.h>

int noconsole;

void
cninit(void)
{
	struct consdev *cp;

	cn_tab = NULL;
	noconsole = 1;
	for (cp = constab; cp->cn_probe; cp++) {
		(*cp->cn_probe)(cp);
		if (cp->cn_pri > CN_DEAD &&
		    (cn_tab == NULL || cp->cn_pri > cn_tab->cn_pri)) {
			cn_tab = cp;
		}
	}
	if (cn_tab) {
		(*cn_tab->cn_init)(cn_tab);
		noconsole = 0;
	}
}

int
cngetc(void)
{

	if (cn_tab)
		return((*cn_tab->cn_getc)(0));
	return(0);
}

int
tcngetc(void)
{
	if (cn_tab)
		return((*cn_tab->cn_getc)(0x80));
	return(0);
}

void
cnputc(int c)
{

	/* Note: the dev_t arguments are not used! */
	if (cn_tab)
		(*cn_tab->cn_putc)(0, c);
}

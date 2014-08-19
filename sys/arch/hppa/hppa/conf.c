/*	$NetBSD: conf.c,v 1.1.10.2 2014/08/20 00:03:04 tls Exp $	*/

/*	$OpenBSD: conf.c,v 1.15 2001/09/28 02:53:13 mickey Exp $	*/

/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 *     @(#)conf.c	7.9 (Berkeley) 5/28/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: conf.c,v 1.1.10.2 2014/08/20 00:03:04 tls Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <dev/cons.h>

cons_decl(pdc);
/*cons_decl(wscons);*/
cons_decl(com);

struct  consdev constab[] = {
	cons_init(pdc),		/* XXX you'd better leave it here for pdc.c */
#if 0
#if NWSCONS1 > 0
	cons_init(wscons),
#endif
#endif
#if NCOM > 0
	cons_init(com),
#endif
	{ .cn_probe = NULL }
};


/*	$NetBSD: hpckbdvar.h,v 1.1 2000/09/21 14:17:29 takemura Exp $ */

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * chip interface
 */
struct hpckbd_if;
struct hpckbd_ic_if {
	void	*hii_ctx;
	int	(*hii_establish) __P((void*, struct hpckbd_if *));
	int	(*hii_poll) __P((void*));
};

#define hpckbd_ic_establish(ic, kbdif) \
	(*(ic)->hii_establish)((ic)->hii_ctx, (kbdif))
#define hpckbd_ic_poll(ic) \
	(*(ic)->hii_poll)((ic)->hii_ctx)

/*
 * hpckbd interface
 */
struct hpckbd_if {
	void	*hi_ctx;
	void	(*hi_input_hook) __P((void*));
	int	(*hi_input) __P((void*, int, int));
};

#define hpckbd_input_hook(i) \
	(*(i)->hi_input_hook)((i)->hi_ctx)
#define hpckbd_input(i, f, a) \
	(*(i)->hi_input)((i)->hi_ctx, (f), (a))

struct hpckbd_attach_args {
	struct hpckbd_ic_if *haa_ic;
};

int	hpckbd_print __P((void*, const char*));
int	hpckbd_cnattach __P((struct hpckbd_ic_if*));

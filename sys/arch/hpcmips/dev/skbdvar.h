/*	$NetBSD: skbdvar.h,v 1.1 1999/12/08 15:49:19 uch Exp $ */

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

typedef struct skbd_controller *skbd_tag_t;

struct skbd_controller {
	/* controller interface */
	void	*skif_v;
	int	(*skif_establish) __P((void*, 
				       int (*)(void*, int, int), 
				       void (*)(void*),
				       void*));
	int	(*skif_poll) __P((void*));

	/* keyboard interface */
	void	*sk_v;
	void	(*sk_input_hook) __P((void*));
	int	(*sk_input) __P((void*, int, int));
};

struct skbd_attach_args {
	skbd_tag_t saa_ic;
};

#define skbdif_establish(c, i, h, a) \
	(*(c)->skif_establish)((c)->skif_v, (i), (h), (a))
#define skbdif_poll(c) \
	(*(c)->skif_poll)((c)->skif_v)

#define skbd_input_hook(c) \
	(*(c)->sk_input_hook)((c)->sk_v)
#define skbd_input(c, f, a) \
	(*(c)->sk_input)((c)->sk_v, (f), (a))

int	skbd_print __P((void*, const char*));
int	skbd_cnattach __P((struct skbd_controller*));



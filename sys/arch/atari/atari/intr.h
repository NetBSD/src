/*	$NetBSD: intr.h,v 1.2 1997/06/06 13:15:57 leo Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Leo Weppelman.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _ATARI_ATARI_INTR_H_
#define _ATARI_ATARI_INTR_H_

#include <machine/cpu.h>	/* XXX: for clockframe */

#define	AUTO_VEC	0x0001	/* We're dealing with an auto-vector	*/
#define	USER_VEC	0x0002	/* We're dealing with an user-vector	*/

#define	FAST_VEC	0x0010	/* Fast, stash right into vector-table	*/
#define	ARG_CLOCKFRAME	0x0020	/* Supply clockframe as an argument	*/

/*
 * Interrupt handler chains.  intr_establish() inserts a handler into
 * the list.  The handler is called with its (single) argument or with a
 * 'standard' clockframe. This depends on 'ih_type'.
 */
typedef int	(*hw_ifun_t) __P((void *, int));

struct intrhand {
	LIST_ENTRY(intrhand)	ih_link;
	hw_ifun_t		ih_fun;
	void			*ih_arg;
	int			ih_type;
	int			ih_pri;
	int			ih_vector;
	u_long			*ih_intrcnt;
};

void		intr_init __P((void));
struct intrhand *intr_establish __P((int, int, int, hw_ifun_t, void *));
int		intr_disestablish __P((struct intrhand *));
void		intr_dispatch __P((struct clockframe));
void		intr_glue __P((void));

/*
 * Exported by intrcnt.h
 */
extern u_long	autovects[];
extern u_long	intrcnt_auto[];
extern u_long	uservects[];
extern u_long	intrcnt_user[];

#endif /* _ATARI_ATARI_INTR_H_ */

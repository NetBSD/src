/*	$NetBSD: isa_machdep.h,v 1.1.1.1 1997/07/15 08:17:39 leo Exp $	*/

/*
 * Copyright (c) 1997 Leo Weppelman.  All rights reserved.
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
 *	This product includes software developed by Leo Weppelman.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#ifndef _ATARI_ISA_MACHDEP_H_
#define _ATARI_ISA_MACHDEP_H_

#include <atari/atari/intr.h>

#define	ISA_IOSTART	0xfff30000	/* XXX: Range with byte frobbing */
#define	ISA_MEMSTART	0xff000000	/* XXX: Hades only	*/

typedef void	*isa_chipset_tag_t;

typedef struct	{
	int		slot;	/* 1/2, determines interrupt line	*/
	int		ipl;	/* ipl requested			*/
	hw_ifun_t	ifunc;	/* function to call			*/
	void		*iarg;	/* argument for 'ifunc'			*/
	struct intrhand	*ihand;	/* save this for disestablishing	*/
} isa_intr_info_t;

/*
 * Functions provided to machine-independent ISA code.
 */
void	isa_attach_hook __P((struct device *, struct device *,
	    struct isabus_attach_args *));
void	*isa_intr_establish __P((isa_chipset_tag_t ic, int irq, int type,
	    int level, hw_ifun_t ih_fun, void *ih_arg));
void	isa_intr_disestablish __P((isa_chipset_tag_t ic, void *handler));

#endif /* _ATARI_ISA_MACHDEP_H_ */

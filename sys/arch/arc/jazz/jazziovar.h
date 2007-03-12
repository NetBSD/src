/*	$NetBSD: jazziovar.h,v 1.6.10.1 2007/03/12 05:46:50 rmind Exp $	*/

/*-
 * Copyright (C) 2000 Shuichiro URATA.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __JAZZIOVAR_H
#define __JAZZIOVAR_H

#include <machine/bus.h>

struct jazzio_attach_args {
	const char *ja_name;
	bus_space_tag_t	ja_bust;
	bus_dma_tag_t	ja_dmat;
	bus_addr_t	ja_addr;
	int		ja_intr;
	int		ja_dma;
};

struct pica_dev {
	struct confargs	ps_ca;
	void *		ps_base;
};

struct jazzio_config {
	int jc_iointr_status_reg;
	void (*jc_set_iointr_mask)(int);
	bus_addr_t jc_dmatlbreg;
	bus_addr_t jc_fdcdmareg;	/* XXX should be obtained from bios */
};

void	jazzio_intr_establish(int, int (*)(void *), void *);
void	jazzio_intr_disestablish(int);
void	jazzio_reset(void);

extern struct jazzio_config *jazzio_conf;
extern struct pica_dev *jazzio_devconfig;

extern struct arc_bus_space jazzio_bus;

#endif

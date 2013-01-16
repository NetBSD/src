/*	$NetBSD: epsocvar.h,v 1.4.2.2 2013/01/16 05:32:46 yamt Exp $ */
/*
 * Copyright (c) 2004 Jesse Off
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
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _EPSOCVAR_H_
#define _EPSOCVAR_H_

#include <sys/conf.h>
#include <sys/device.h>
#include <sys/queue.h>

#include <sys/bus.h>

struct epgpio_softc;

struct epsoc_attach_args {
	bus_space_tag_t		sa_iot;		/* Bus tag */
	bus_dma_tag_t		sa_dmat;
	bus_addr_t		sa_addr;	/* i/o address  */
	bus_size_t		sa_size;
	int			sa_intr;
	uint32_t		sa_hclk;
	uint32_t		sa_pclk;
	struct epgpio_softc	*sa_gpio;
};

struct epsoc_softc {
	uint32_t		sc_fclk;
	uint32_t		sc_hclk;
	uint32_t		sc_pclk;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_dma_tag_t		sc_dmat;
};

extern struct epsoc_softc *epsoc_sc;

#endif /* _EPSOCVAR_H_ */

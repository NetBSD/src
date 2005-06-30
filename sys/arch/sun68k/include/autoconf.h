/*	$NetBSD: autoconf.h,v 1.5 2005/06/30 17:03:54 drochner Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass, Gordon W. Ross, and Matthew Fredette.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Autoconfiguration information.
 * (machdep parts of driver/kernel interface)
 */

#include <machine/bus.h>
#include <machine/promlib.h>

/* Attach arguments presented by mainbus_attach() */
struct mainbus_attach_args {
	bus_space_tag_t	ma_bustag;	/* parent bus tag */
	bus_dma_tag_t	ma_dmatag;
	const char	*ma_name;	/* device name */
	bus_addr_t	ma_paddr;	/* physical address */
	int		ma_pri;		/* priority (IPL) */
};

/* Aliases for other buses */
#define	obio_attach_args	mainbus_attach_args
#define	oba_bustag		ma_bustag
#define	oba_dmatag		ma_dmatag
#define	oba_name		ma_name
#define	oba_paddr		ma_paddr
#define	oba_pri			ma_pri

#define	obmem_attach_args	mainbus_attach_args
#define	obma_bustag		ma_bustag
#define	obma_dmatag		ma_dmatag
#define	obma_name		ma_name
#define	obma_paddr		ma_paddr
#define	obma_pri		ma_pri

/* Locator aliases */
#define	LOCATOR_OPTIONAL	(0)
#define	LOCATOR_REQUIRED	(1)
#define	LOCATOR_FORBIDDEN	(2)

int sun68k_bus_search(struct device *, struct cfdata *,
		      const locdesc_t *, void *);
int sun68k_bus_print(void *, const char *);


/*	$NetBSD: board.h,v 1.4 2006/10/02 08:13:53 gdamore Exp $	*/

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Garrett D'Amore for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 

#ifndef _EVBMIPS_ALCHEMY_BOARD_H
#define	_EVBMIPS_ALCHEMY_BOARD_H

#include <dev/pci/pcivar.h>

struct alchemy_board {
	const char		*ab_name;
	const struct obiodev	*ab_devices;
	void		(*ab_init)(void);
	int		(*ab_pci_intr_map)(struct pci_attach_args *,
					   pci_intr_handle_t *);

	void		(*ab_reboot)(void);
	void		(*ab_poweroff)(void);

	struct aupcmcia_machdep	*ab_pcmcia;

	const struct auspi_machdep *(*ab_spi)(bus_addr_t);

	/*
	 * XXX: csb250 (and perhaps others) will require pci_idsel
	 * entry point
	 */
};

const struct alchemy_board *board_info(void);

#endif	/* _EVBMIPS_ALCHEMY_BOARD_H */

/*	$NetBSD: pci_milan.c,v 1.1 2001/05/15 14:14:49 leo Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <machine/bswap.h>

int
pci_bus_maxdevs(pc, busno)
	pci_chipset_tag_t pc;
	int busno;
{
	return (6);
}

/*
 * These are defined in locore.s:
 */
pcireg_t	milan_pci_confread(pcitag_t);
void		milan_pci_confwrite(u_long, pcireg_t);
u_long		plx_status;

pcireg_t
pci_conf_read(pc, tag, reg)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
{
	u_long		data;

	data = bswap32(milan_pci_confread(tag | reg));
	if ((plx_status) & 0xf9000000) {
		/*
		 * Access error, assume nothing there...
		 */
		data = 0xffffffff;
	}
	return(data);
}


void
pci_conf_write(pc, tag, reg, data)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
	pcireg_t data;
{
	milan_pci_confwrite(tag | reg, bswap32(data));
}

void *
pci_intr_establish(pc, ih, level, ih_fun, ih_arg)
	pci_chipset_tag_t	pc;
	pci_intr_handle_t	ih;
	int			level;
	int			(*ih_fun) __P((void *));
	void			*ih_arg;
{
	printf("pci_intr_establish: Not yet implemented\n");
	return NULL;
}

void
pci_intr_disestablish(pc, cookie)
	pci_chipset_tag_t pc;
	void *cookie;
{
}

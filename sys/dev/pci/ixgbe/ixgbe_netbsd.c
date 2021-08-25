/* $NetBSD: ixgbe_netbsd.c,v 1.17 2021/08/25 09:06:02 msaitoh Exp $ */
/*
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Coyote Point Systems, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ixgbe_netbsd.c,v 1.17 2021/08/25 09:06:02 msaitoh Exp $");

#include <sys/param.h>

#include <sys/atomic.h>
#include <sys/bus.h>
#include <sys/condvar.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/mbuf.h>
#include <sys/mutex.h>
#include <sys/queue.h>
#include <sys/workqueue.h>
#include <dev/pci/pcivar.h>

#include "ixgbe.h"

void
ixgbe_dma_tag_destroy(ixgbe_dma_tag_t *dt)
{
	kmem_free(dt, sizeof(*dt));
}

int
ixgbe_dma_tag_create(bus_dma_tag_t dmat, bus_size_t alignment,
    bus_size_t boundary, bus_size_t maxsize, int nsegments,
    bus_size_t maxsegsize, int flags, ixgbe_dma_tag_t **dtp)
{
	ixgbe_dma_tag_t *dt;

	*dtp = NULL;

	dt = kmem_zalloc(sizeof(*dt), KM_SLEEP);
	dt->dt_dmat = dmat;
	dt->dt_alignment = alignment;
	dt->dt_boundary = boundary;
	dt->dt_maxsize = maxsize;
	dt->dt_nsegments = nsegments;
	dt->dt_maxsegsize = maxsegsize;
	dt->dt_flags = flags;
	*dtp = dt;

	return 0;
}

void
ixgbe_dmamap_destroy(ixgbe_dma_tag_t *dt, bus_dmamap_t dmam)
{
	bus_dmamap_destroy(dt->dt_dmat, dmam);
}

void
ixgbe_dmamap_sync(ixgbe_dma_tag_t *dt, bus_dmamap_t dmam, int ops)
{
	bus_dmamap_sync(dt->dt_dmat, dmam, 0, dt->dt_maxsize, ops);
}

void
ixgbe_dmamap_unload(ixgbe_dma_tag_t *dt, bus_dmamap_t dmam)
{
	bus_dmamap_unload(dt->dt_dmat, dmam);
}

int
ixgbe_dmamap_create(ixgbe_dma_tag_t *dt, int flags, bus_dmamap_t *dmamp)
{
	return bus_dmamap_create(dt->dt_dmat, dt->dt_maxsize, dt->dt_nsegments,
	    dt->dt_maxsegsize, dt->dt_boundary, flags, dmamp);
}


struct mbuf *
ixgbe_getcl(void)
{
	struct mbuf *m;

	MGETHDR(m, M_DONTWAIT, MT_DATA);

	if (m == NULL)
		return NULL;

	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return NULL;
	}

	return m;
}

void
ixgbe_pci_enable_busmaster(pci_chipset_tag_t pc, pcitag_t tag)
{
	pcireg_t	pci_cmd_word;

	pci_cmd_word = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	if (!(pci_cmd_word & PCI_COMMAND_MASTER_ENABLE)) {
		pci_cmd_word |= PCI_COMMAND_MASTER_ENABLE;
		pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, pci_cmd_word);
	}
}

u_int
atomic_load_acq_uint(volatile u_int *p)
{
	return atomic_load_acquire(p);
}

void
ixgbe_delay(unsigned int us)
{

	if (__predict_false(cold))
		delay(us);
	else if ((us / 1000) >= hztoms(1)) {
		/*
		 * Wait at least two clock ticks so we know the time has
		 * passed.
		 */
		kpause("ixgdly", false, mstohz(us / 1000) + 1, NULL);
	} else
		delay(us);
}

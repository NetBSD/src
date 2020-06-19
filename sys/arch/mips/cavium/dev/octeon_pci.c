/*	$NetBSD: octeon_pci.c,v 1.4 2020/06/19 02:23:43 simonb Exp $	*/

/*
 * Copyright (c) 2007, 2008 Internet Initiative Japan, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_pci.c,v 1.4 2020/06/19 02:23:43 simonb Exp $");

#include "opt_octeon.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <mips/cavium/octeonvar.h>
#include <mips/cavium/dev/octeon_ciureg.h>
#include <mips/cavium/dev/octeon_npireg.h>

/*
 * In OCTEON, some infrequent, error interrupts (RML) are handled with PCI
 * interrupt.  Hence, here.
 */

void			octpci_bootstrap(struct octeon_config *);
static void		octpci_init(void);

#ifdef CNMAC_DEBUG
static int		octpci_intr_rml(void *);
static void		*octpci_intr_rml_ih;
#endif

void
octpci_bootstrap(struct octeon_config *mcp)
{
	octpci_init();
}

static void
octpci_init(void)
{
#ifdef CNMAC_DEBUG
	octpci_intr_rml_ih = octeon_intr_establish( CIU_INT_RML, IPL_NET,
	    octpci_intr_rml, NULL);
#endif
}

#ifdef CNMAC_DEBUG
int octpci_intr_rml_verbose;

void			octgmx_intr_rml_gmx0(void *);
void			octgmx_intr_rml_gmx1(void *);
void			octasx_intr_rml(void *);
void			octipd_intr_rml(void *);
void			octpip_intr_rml(void *);
void			octpow_intr_rml(void *);
void			octpko_intr_rml(void *);
void			octfpa_intr_rml(void *);

static int
octpci_intr_rml(void *arg)
{
	uint64_t block;

	block = octeon_read_csr(NPI_RSL_INT_BLOCKS);
	if (octpci_intr_rml_verbose)
		printf("%s: block=0x%016" PRIx64 "\n", __func__, block);
	if (ISSET(block, NPI_RSL_INT_BLOCKS_GMX0))
		octgmx_intr_rml_gmx0(arg);
#ifdef notyet
	if (ISSET(block, NPI_RSL_INT_BLOCKS_GMX1))
		octgmx_intr_rml_gmx1(arg);
#endif
	if (ISSET(block, NPI_RSL_INT_BLOCKS_ASX0))
		octasx_intr_rml(arg);
	if (ISSET(block, NPI_RSL_INT_BLOCKS_IPD))
		octipd_intr_rml(arg);
	if (ISSET(block, NPI_RSL_INT_BLOCKS_PIP))
		octpip_intr_rml(arg);
	if (ISSET(block, NPI_RSL_INT_BLOCKS_POW))
		octpow_intr_rml(arg);
	if (ISSET(block, NPI_RSL_INT_BLOCKS_PKO))
		octpko_intr_rml(arg);
	if (ISSET(block, NPI_RSL_INT_BLOCKS_FPA))
		octfpa_intr_rml(arg);
	return 1;
}
#endif

/*	$NetBSD: octeon_pci.c,v 1.2.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: octeon_pci.c,v 1.2.18.2 2017/12/03 11:36:27 jdolecek Exp $");

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

void			octeon_pci_bootstrap(struct octeon_config *);
static void		octeon_pci_init(void);

#ifdef OCTEON_ETH_DEBUG
static int		octeon_pci_intr_rml(void *);
static void		*octeon_pci_intr_rml_ih;
#endif

void
octeon_pci_bootstrap(struct octeon_config *mcp)
{
	octeon_pci_init();
}

static void
octeon_pci_init(void)
{
#ifdef OCTEON_ETH_DEBUG
	octeon_pci_intr_rml_ih = octeon_intr_establish(
	    ffs64(CIU_INTX_SUM0_RML) - 1, IPL_NET, octeon_pci_intr_rml, NULL);
#endif
}

#ifdef OCTEON_ETH_DEBUG
int octeon_pci_intr_rml_verbose;

void			octeon_gmx_intr_rml_gmx0(void *);
void			octeon_gmx_intr_rml_gmx1(void *);
void			octeon_asx_intr_rml(void *);
void			octeon_ipd_intr_rml(void *);
void			octeon_pip_intr_rml(void *);
void			octeon_pow_intr_rml(void *);
void			octeon_pko_intr_rml(void *);
void			octeon_fpa_intr_rml(void *);

static int
octeon_pci_intr_rml(void *arg)
{
	uint64_t block;

	block = octeon_read_csr(NPI_RSL_INT_BLOCKS);
	if (octeon_pci_intr_rml_verbose)
		printf("%s: block=0x%016" PRIx64 "\n", __func__, block);
	if (ISSET(block, NPI_RSL_INT_BLOCKS_GMX0))
		octeon_gmx_intr_rml_gmx0(arg);
#ifdef notyet
	if (ISSET(block, NPI_RSL_INT_BLOCKS_GMX1))
		octeon_gmx_intr_rml_gmx1(arg);
#endif
	if (ISSET(block, NPI_RSL_INT_BLOCKS_ASX0))
		octeon_asx_intr_rml(arg);
	if (ISSET(block, NPI_RSL_INT_BLOCKS_IPD))
		octeon_ipd_intr_rml(arg);
	if (ISSET(block, NPI_RSL_INT_BLOCKS_PIP))
		octeon_pip_intr_rml(arg);
	if (ISSET(block, NPI_RSL_INT_BLOCKS_POW))
		octeon_pow_intr_rml(arg);
	if (ISSET(block, NPI_RSL_INT_BLOCKS_PKO))
		octeon_pko_intr_rml(arg);
	if (ISSET(block, NPI_RSL_INT_BLOCKS_FPA))
		octeon_fpa_intr_rml(arg);
	return 1;
}
#endif

/*	$NetBSD: rmixl_obio.c,v 1.8 2022/01/22 15:08:11 skrll Exp $	*/

/*
 * Copyright (c) 2001, 2002, 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * On-board device autoconfiguration support for RMI {XLP, XLR, XLS} chips
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rmixl_obio.c,v 1.8 2022/01/22 15:08:11 skrll Exp $");

#include "locators.h"
#include "pci.h"
#define _MIPS_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <mips/int_fmtio.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_intr.h>
#include <mips/rmi/rmixl_obiovar.h>
#include <mips/rmi/rmixl_pcievar.h>

#include <evbmips/rmixl/autoconf.h>

#ifdef OBIO_DEBUG
int obio_rmixl_debug = OBIO_DEBUG;
# define DPRINTF(x)	do { if (obio_rmixl_debug) printf x ; } while (0)
#else
# define DPRINTF(x)
#endif

static int  obio_match(device_t, cfdata_t, void *);
static void obio_attach(device_t, device_t, void *);
static int  obio_print(void *, const char *);
static int  obio_search(device_t, cfdata_t, const int *, void *);
static void obio_bus_init(struct obio_softc *);
static void obio_dma_init_64(bus_dma_tag_t);
static int  rmixl_addr_error_intr(void *);


CFATTACH_DECL_NEW(obio_rmixl, sizeof(struct obio_softc),
    obio_match, obio_attach, NULL, NULL);

int obio_found;

static int
obio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *aa = aux;

	if (obio_found == 0)
		if (strncmp(aa->ma_name, cf->cf_name, strlen(cf->cf_name)) == 0)
			return 1;

	return 0;
}

static void
obio_attach(device_t parent, device_t self, void *aux)
{
	struct obio_softc *sc = device_private(self);
	bus_addr_t ba;

	obio_found = 1;
	sc->sc_dev = self;

	ba = (bus_addr_t)rmixl_configuration.rc_io_pbase;
	KASSERT(ba != 0);

	obio_bus_init(sc);

	aprint_normal(" addr %#"PRIxBUSADDR" size %#"PRIxBUSSIZE"\n",
		ba, (bus_size_t)RMIXL_IO_DEV_SIZE);
	aprint_naive("\n");

	/*
	 * Attach on-board devices as specified in the kernel config file.
	 */
	config_search(self, NULL,
	    CFARGS(.search = obio_search));
}

static int
obio_print(void *aux, const char *pnp)
{
	struct obio_attach_args *obio = aux;

	if (obio->obio_addr != OBIOCF_ADDR_DEFAULT) {
		aprint_normal(" addr %#"PRIxBUSADDR, obio->obio_addr);
		if (obio->obio_size != OBIOCF_SIZE_DEFAULT)
			aprint_normal("-%#"PRIxBUSADDR,
				obio->obio_addr + (obio->obio_size - 1));
	}
	if (obio->obio_mult != OBIOCF_MULT_DEFAULT)
		aprint_normal(" mult %d", obio->obio_mult);
	if (obio->obio_intr != OBIOCF_INTR_DEFAULT)
		aprint_normal(" intr %d", obio->obio_intr);
	if (obio->obio_tmsk != OBIOCF_TMSK_DEFAULT)
		aprint_normal(" tmsk %d", obio->obio_tmsk);

	return (UNCONF);
}

static int
obio_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct obio_softc *sc = device_private(parent);
	struct obio_attach_args obio;

	obio.obio_eb_bst = sc->sc_eb_bst;
	obio.obio_el_bst = sc->sc_el_bst;
	obio.obio_addr = cf->cf_loc[OBIOCF_ADDR];
	obio.obio_size = cf->cf_loc[OBIOCF_SIZE];
	obio.obio_mult = cf->cf_loc[OBIOCF_MULT];
	obio.obio_intr = cf->cf_loc[OBIOCF_INTR];
	obio.obio_tmsk = cf->cf_loc[OBIOCF_TMSK];
	obio.obio_29bit_dmat = sc->sc_29bit_dmat;
	obio.obio_32bit_dmat = sc->sc_32bit_dmat;
	obio.obio_64bit_dmat = sc->sc_64bit_dmat;

	if (config_probe(parent, cf, &obio))
		config_attach(parent, cf, &obio, obio_print, CFARGS_NONE);

	return 0;
}

static void
obio_bus_init(struct obio_softc *sc)
{
	struct rmixl_config *rcp = &rmixl_configuration;
	static int done = 0;
	int error;

	if (done)
		return;
	done = 1;

	/* obio (devio) space, Big Endian */
	if (rcp->rc_obio_eb_memt.bs_cookie == 0)
		rmixl_obio_eb_bus_mem_init(&rcp->rc_obio_eb_memt, rcp);

	/* obio (devio) space, Little Endian */
	if (rcp->rc_obio_el_memt.bs_cookie == 0)
		rmixl_obio_el_bus_mem_init(&rcp->rc_obio_el_memt, rcp);

	/* dma space for all memory, including >= 4GB */
	if (rcp->rc_dma_tag._cookie == 0)
		obio_dma_init_64(&rcp->rc_dma_tag);
	rcp->rc_64bit_dmat = &rcp->rc_dma_tag;

	/* dma space for addr < 4GB */
	if (rcp->rc_32bit_dmat == NULL) {
		error = bus_dmatag_subregion(rcp->rc_64bit_dmat,
		    0, (bus_addr_t)1 << 32, &rcp->rc_32bit_dmat, 0);
		if (error)
			panic("%s: failed to create 32bit dma tag: %d",
			    __func__, error);
	}

	/* dma space for addr < 512MB */
	if (rcp->rc_29bit_dmat == NULL) {
		error = bus_dmatag_subregion(rcp->rc_32bit_dmat,
		    0, (bus_addr_t)1 << 29, &rcp->rc_29bit_dmat, 0);
		if (error)
			panic("%s: failed to create 29bit dma tag: %d",
			    __func__, error);
	}

	sc->sc_base = (bus_addr_t)rcp->rc_io_pbase;
	sc->sc_size = (bus_size_t)RMIXL_IO_DEV_SIZE;
	sc->sc_eb_bst = (bus_space_tag_t)&rcp->rc_obio_eb_memt;
	sc->sc_el_bst = (bus_space_tag_t)&rcp->rc_obio_el_memt;
	sc->sc_29bit_dmat = rcp->rc_29bit_dmat;
	sc->sc_32bit_dmat = rcp->rc_32bit_dmat;
	sc->sc_64bit_dmat = rcp->rc_64bit_dmat;
}

static void
obio_dma_init_64(bus_dma_tag_t t)
{
	t->_cookie = t;
	t->_wbase = 0;
	t->_bounce_alloc_lo = 0;
	t->_bounce_alloc_hi = 0;
	t->_dmamap_ops = mips_bus_dmamap_ops;
	t->_dmamem_ops = mips_bus_dmamem_ops;
	t->_dmatag_ops = mips_bus_dmatag_ops;
}

void
rmixl_addr_error_init(void)
{
	uint32_t r;

	/*
	 * activate error addr detection on all (configurable) devices
	 * preserve reserved bit fields
	 * note some of these bits are read-only (writes are ignored)
	 */
	r = RMIXL_IOREG_READ(RMIXL_ADDR_ERR_DEVICE_MASK);
	r |= ~(__BITS(19,16) | __BITS(10,9) | __BITS(7,5));
	RMIXL_IOREG_WRITE(RMIXL_ADDR_ERR_DEVICE_MASK, r);

	/*
	 * enable the address error interrupts
	 * "upgrade" cache and CPU errors to A1
	 */
#define _ADDR_ERR_DEVSTAT_A1	(__BIT(8) | __BIT(1) | __BIT(0))
#define _ADDR_ERR_RESV		\
		(__BITS(31,21) | __BITS(15,14) | __BITS(10,9) | __BITS(7,2))
#define _BITERR_INT_EN_RESV	(__BITS(31,8) | __BIT(4))

	r = RMIXL_IOREG_READ(RMIXL_ADDR_ERR_AERR0_EN);
	r &= _ADDR_ERR_RESV;
	r |= ~_ADDR_ERR_RESV;
	RMIXL_IOREG_WRITE(RMIXL_ADDR_ERR_AERR0_EN, r);

	r = RMIXL_IOREG_READ(RMIXL_ADDR_ERR_AERR0_UPG);
	r &= _ADDR_ERR_RESV;
	r |= _ADDR_ERR_DEVSTAT_A1;
	RMIXL_IOREG_WRITE(RMIXL_ADDR_ERR_AERR0_UPG, r);

	/*
	 * clear the log regs and the dev stat (interrupt status) regs
	 * "Write any value to bit[0] to clear"
	 */
	r = RMIXL_IOREG_READ(RMIXL_ADDR_ERR_AERR1_CLEAR);
	RMIXL_IOREG_WRITE(RMIXL_ADDR_ERR_AERR1_CLEAR, r);

	/*
	 * enable the double bit error interrupts
	 * (assume reserved bits, which are read-only,  are ignored)
	 */
	r = RMIXL_IOREG_READ(RMIXL_ADDR_ERR_BITERR_INT_EN);
	r &= _BITERR_INT_EN_RESV;
	r |= __BITS(7,5);
	RMIXL_IOREG_WRITE(RMIXL_ADDR_ERR_BITERR_INT_EN, r);

	/*
	 * establish address error ISR
	 * XXX assuming "int 16 (bridge_tb)" is our irq
	 * XXX is true for XLS family only
	 */
	if (cpu_rmixls(mips_options.mips_cpu))
		rmixl_intr_establish(16, 1, IPL_HIGH,
			RMIXL_TRIG_LEVEL, RMIXL_POLR_HIGH,
			rmixl_addr_error_intr, NULL, false);
}

int
rmixl_addr_error_check(void)
{
	uint32_t aerr0_devstat;
	uint32_t aerr0_log1;
	uint32_t aerr0_log2;
	uint32_t aerr0_log3;
	uint32_t aerr1_devstat;
	uint32_t aerr1_log1;
	uint32_t aerr1_log2;
	uint32_t aerr1_log3;
	uint32_t sbe_counts;
	uint32_t dbe_counts;

	aerr0_devstat = RMIXL_IOREG_READ(RMIXL_ADDR_ERR_AERR0_DEVSTAT);
	aerr0_log1 = RMIXL_IOREG_READ(RMIXL_ADDR_ERR_AERR0_LOG1);
	aerr0_log2 = RMIXL_IOREG_READ(RMIXL_ADDR_ERR_AERR0_LOG2);
	aerr0_log3 = RMIXL_IOREG_READ(RMIXL_ADDR_ERR_AERR0_LOG3);

	aerr1_devstat = RMIXL_IOREG_READ(RMIXL_ADDR_ERR_AERR1_DEVSTAT);
	aerr1_log1 = RMIXL_IOREG_READ(RMIXL_ADDR_ERR_AERR1_LOG1);
	aerr1_log2 = RMIXL_IOREG_READ(RMIXL_ADDR_ERR_AERR1_LOG2);
	aerr1_log3 = RMIXL_IOREG_READ(RMIXL_ADDR_ERR_AERR1_LOG3);

	sbe_counts = RMIXL_IOREG_READ(RMIXL_ADDR_ERR_SBE_COUNTS);
	dbe_counts = RMIXL_IOREG_READ(RMIXL_ADDR_ERR_DBE_COUNTS);

	if (aerr0_log1|aerr0_log2|aerr0_log3
	   |aerr1_log1|aerr1_log2|aerr1_log3
	   |dbe_counts) {
		printf("aerr0: stat %#x, logs: %#x, %#x, %#x\n",
			aerr0_devstat, aerr0_log1, aerr0_log2, aerr0_log2);
		printf("aerr1: stat %#x, logs: %#x, %#x, %#x\n",
			aerr1_devstat, aerr1_log1, aerr1_log2, aerr1_log2);
		printf("1-bit errors: %#x, 2-bit errors: %#x\n",
			sbe_counts, dbe_counts);
		return 1;
	}
	return 0;
}

static int
rmixl_addr_error_intr(void *arg)
{
	int err;

	err = rmixl_addr_error_check();
	if (err != 0) {
#if DDB
		printf("%s\n", __func__);
		Debugger();
#endif
		panic("Address Error");
	}
	return 1;
}

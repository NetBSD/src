/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: rmixl_nand_pci.c,v 1.1.2.4 2014/02/14 18:38:16 matt Exp $");

#include <sys/param.h>
#include <sys/condvar.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/nand/nand.h>
#include <dev/nand/onfi.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>

#include "locators.h"

static	int xlnand_pci_match(device_t, cfdata_t, void *);
static	void xlnand_pci_attach(device_t, device_t, void *);
static	int xlnand_pci_detach(device_t, int);

static	int xlnand_read_page(device_t, size_t, uint8_t *);
static	int xlnand_program_page(device_t, size_t, const uint8_t *);

static	void xlnand_select(device_t, bool);
static	void xlnand_command(device_t, uint8_t);
static	void xlnand_address(device_t, uint8_t);
static	void xlnand_busy(device_t);
static	void xlnand_read_buf(device_t, void *, size_t);
static	void xlnand_write_buf(device_t, const void *, size_t);

static	void xlnand_read_1(device_t, uint8_t *);
static	void xlnand_read_2(device_t, uint16_t *);
static	void xlnand_write_1(device_t, uint8_t);
static	void xlnand_write_2(device_t, uint16_t);

static	int xlnand_intr(void *);

struct	xlnand_chip {
	device_t xlch_nanddev;
	kcondvar_t xlch_cv_ready;

	uint32_t xlch_cmds;
	uint64_t xlch_addrs;
	uint32_t xlch_data;
	uint32_t xlch_int_mask;
	uint8_t xlch_cmdshift;
	uint8_t xlch_addrshift;
	uint8_t xlch_num;
	uint8_t xlch_datalen;
	uint8_t xlch_chipnum;

	struct nand_interface xlch_nand_if;
	char xlch_wmesg[16];
};

struct	xlnand_softc {
	device_t xlsc_dev;
	bus_dma_tag_t xlsc_dmat;
	bus_space_tag_t xlsc_bst;
	bus_space_handle_t xlsc_bsh;
	void *xlsc_ih;
	uint8_t xlsc_buswidth;

	kcondvar_t xlsc_cv_available;
	struct xlnand_chip *xlsc_active_chip;

	bus_dma_segment_t xlsc_xferseg;
	bus_dmamap_t xlsc_xfermap;
	void *xlsc_xferbuf;

	struct xlnand_chip xlsc_chips[8];

	kmutex_t xlsc_intr_lock __aligned(64);
	kmutex_t xlsc_wait_lock __aligned(64);
};

#define CMDSEQ1(a,b)		(((ONFI_ ## b) << 8) | RMIXLP_NAND_CMD_ ## a)
#define CMDSEQ2(a,b,c)		(((ONFI_ ## c) << 16) | CMDSEQ1(a,b))
#define CMDSEQ3(a,b,c,d)	(((ONFI_ ## d) << 14) | CMDSEQ2(a,b,c))

static const uint32_t xlnand_cmdseqs[] = {
	CMDSEQ1(SEQ_0, RESET),
	//CMDSEQ1(SEQ_0, SYNCRONOUS_RESET),
	CMDSEQ1(SEQ_1, READ_ID),
	CMDSEQ1(SEQ_2, READ_UNIQUE_ID),
	CMDSEQ1(SEQ_2, READ_PARAMETER_PAGE),
	CMDSEQ1(SEQ_2, GET_FEATURES),
	CMDSEQ1(SEQ_3, SET_FEATURES),
	//CMDSEQ1(SEQ_17, SET_FEATURES2),
	CMDSEQ1(SEQ_4, READ_STATUS),
	//CMDSEQ1(SEQ_5, SELECT_LUN_WITH_STATUS),
	CMDSEQ2(SEQ_6, CHANGE_READ_COLUMN, CHANGE_READ_COLUMN_START),
	//CMDSEQ2(SEQ_7, SELECT_CACHE_REGISTER, CHANGE_READ_COLUMN_START),
	//CMDSEQ1(SEQ_8, CHANGE_WRITE_COLUMN),
	//CMDSEQ1(SEQ_12, CHANGE_ROW_ADDRESS),
	CMDSEQ2(SEQ_10, READ, READ_START),
	CMDSEQ2(SEQ_10, READ, READ_CACHE_RANDOM),
	CMDSEQ1(SEQ_11, READ_CACHE_SEQUENTIAL),
	CMDSEQ1(SEQ_11, READ_CACHE_END),
	CMDSEQ2(SEQ_12, READ, READ_INTERLEAVED),
	//CMDSEQ2(SEQ_15, READ, READ, READ_START),
	CMDSEQ2(SEQ_12, PAGE_PROGRAM, PAGE_PROGRAM_START),
	CMDSEQ1(SEQ_13, PAGE_PROGRAM),
	CMDSEQ2(SEQ_12, PAGE_PROGRAM, PAGE_CACHE_PROGRAM),
	CMDSEQ2(SEQ_12, PAGE_PROGRAM, PAGE_PROGRAM_INTERLEAVED),
	//CMDSEQ1(SEQ_0, WRITE_PAGE),
	//CMDSEQ1(SEQ_0, WRITE_PAGE_CACHE),
	//CMDSEQ1(SEQ_0, WRITE_PAGE_INTERLEAVED),
	CMDSEQ2(SEQ_10, READ, READ_COPYBACK),
	CMDSEQ2(SEQ_9, COPYBACK_PROGRAM, COPYBACK_PROGRAM_START),
	CMDSEQ2(SEQ_12, COPYBACK_PROGRAM, COPYBACK_PROGRAM_INTERLEAVED),
	CMDSEQ1(SEQ_13, COPYBACK_PROGRAM),
	CMDSEQ2(SEQ_14, BLOCK_ERASE, BLOCK_ERASE_START),
	CMDSEQ2(SEQ_14, BLOCK_ERASE, BLOCK_ERASE_INTERLEAVED),
};

static inline uint32_t
xlnand_read_4(struct xlnand_softc *xlsc, bus_size_t off)
{
	return bus_space_read_4(xlsc->xlsc_bst, xlsc->xlsc_bsh, off);
}

static inline void
xlnand_write_4(struct xlnand_softc *xlsc, bus_size_t off, uint32_t v)
{
	bus_space_write_4(xlsc->xlsc_bst, xlsc->xlsc_bsh, off, v);
}

CFATTACH_DECL_NEW(xlnand_pci, sizeof(struct xlnand_softc),
    xlnand_pci_match, xlnand_pci_attach, xlnand_pci_detach, NULL);

static int
xlnand_pci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct pci_attach_args * const pa = aux;

	if (pa->pa_id == PCI_ID_CODE(PCI_VENDOR_NETLOGIC, PCI_PRODUCT_NETLOGIC_XLP_NAND))
		return 1;

        return 0;
}

static void
xlnand_pci_attach(device_t parent, device_t self, void *aux)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	struct pci_attach_args * const pa = aux;
	struct xlnand_softc * const xlsc = device_private(self);
	cfdata_t cf = device_cfdata(self);
	const char * const xname = device_xname(self);
	uint8_t csmask = (cf->cf_flags & 0xff ? cf->cf_flags & 0xff : 1);
	uint32_t v;

	xlsc->xlsc_dev = self;
	xlsc->xlsc_bst = &rcp->rc_pci_ecfg_eb_memt;

	mutex_init(&xlsc->xlsc_intr_lock, MUTEX_DEFAULT, IPL_BIO);
	mutex_init(&xlsc->xlsc_wait_lock, MUTEX_DEFAULT, IPL_SOFTBIO);
	cv_init(&xlsc->xlsc_cv_available, xname);

	/*
	 * Why isn't this accessible via a BAR?
	 */
	if (bus_space_subregion(xlsc->xlsc_bst, rcp->rc_pci_ecfg_eb_memh,
		    pa->pa_tag, 0, &xlsc->xlsc_bsh)) {
		aprint_error(": can't map registers\n");
		return;
	}

	aprint_normal(": XLP NAND Controller\n");

	/*
	 * If a NAND is using non-0 RDY/BSY signals, we need to take control
	 * of those from GPIO.
	 */
	uint32_t r = xlnand_read_4(xlsc, RMIXLP_NAND_RDYBSY_SEL);
	for (r >>= 3; r != 0; r >>= 3) {
		u_int rdybsy = r & 7;
		if (rdybsy != 0) {
			rcp->rc_gpio_available &= ~__BIT(33 + rdybsy);
		}
	}

	/*
	 * determine buswidth
	 */
	v = bus_space_read_4(xlsc->xlsc_bst, xlsc->xlsc_bsh, RMIXLP_NAND_CTRL);
	if (v & RMIXLP_NAND_CTRL_IOW) {
		xlsc->xlsc_buswidth = 2;	/* 16 bit */
	} else {
		xlsc->xlsc_buswidth = 1;	/* 8 bit */
	}
	aprint_debug_dev(self, "bus width %d bits\n", 8 * xlsc->xlsc_buswidth);

	pci_intr_handle_t pcih;

	pci_intr_map(pa, &pcih);

	if (pci_intr_establish(pa->pa_pc, pcih, IPL_BIO, xlnand_intr, xlsc) == NULL) {
		aprint_error_dev(self, "failed to establish interrupt\n");
	} else {
		const char * const intrstr = pci_intr_string(pa->pa_pc, pcih);
		aprint_normal_dev(self, "interrupting at %s\n", intrstr);
	}

	for (size_t i = 0; (1 << i) <= csmask; i++) {
		struct xlnand_chip * const xlch = &xlsc->xlsc_chips[i];
		if ((csmask & (1 << i)) == 0)
			continue;
		xlch->xlch_chipnum = i;
		nand_init_interface(&xlch->xlch_nand_if);
		xlch->xlch_nand_if.select = xlnand_select;
		xlch->xlch_nand_if.command = xlnand_command;
		xlch->xlch_nand_if.address = xlnand_address;
		xlch->xlch_nand_if.read_buf_1 = xlnand_read_buf;
		xlch->xlch_nand_if.read_buf_2 = xlnand_read_buf;
		xlch->xlch_nand_if.read_1 = xlnand_read_1;
		xlch->xlch_nand_if.read_2 = xlnand_read_2;
		xlch->xlch_nand_if.write_buf_1 = xlnand_write_buf;
		xlch->xlch_nand_if.write_buf_2 = xlnand_write_buf;
		xlch->xlch_nand_if.write_1 = xlnand_write_1;
		xlch->xlch_nand_if.write_2 = xlnand_write_2;
		xlch->xlch_nand_if.read_page = xlnand_read_page;
		xlch->xlch_nand_if.busy = xlnand_busy;
		xlch->xlch_nand_if.program_page = xlnand_program_page;

		xlch->xlch_nand_if.ecc.necc_code_size = 3;
		xlch->xlch_nand_if.ecc.necc_block_size = 256;

		snprintf(xlch->xlch_wmesg, sizeof(xlch->xlch_wmesg),
		    "%s#%zu", xname, i);

		cv_init(&xlch->xlch_cv_ready, xlch->xlch_wmesg);

		/*
		 * reset to get NAND into known state
		 */
		KASSERT(xlsc->xlsc_active_chip == NULL);
		xlnand_select(self, true);
		KASSERT(xlsc->xlsc_active_chip != NULL);
		xlnand_command(self, ONFI_RESET);
		xlnand_busy(self);
		xlnand_select(self, false);
		KASSERT(xlsc->xlsc_active_chip == NULL);
	}

	if (! pmf_device_register1(self, NULL, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	for (size_t i = 0; (1 << i) <= csmask; i++) {
		struct xlnand_chip * const xlch = &xlsc->xlsc_chips[i];
		if ((csmask & (1 << i)) == 0 || 1)
			continue;
		xlch->xlch_nanddev = nand_attach_mi(&xlch->xlch_nand_if, self);
	}
}

static int
xlnand_pci_detach(device_t self, int flags)
{
	struct xlnand_softc * const xlsc = device_private(self);
	int rv = 0;

	pmf_device_deregister(self);

	for (size_t i = 0; i < __arraycount(xlsc->xlsc_chips); i++) {
		struct xlnand_chip * const xlch = &xlsc->xlsc_chips[i];
		if (xlch->xlch_nanddev != NULL) {
			rv = config_detach(xlch->xlch_nanddev, flags);
		}
	}

	return rv;
}
/*
 * Given the advanced controller, we need to collect the commands and
 * addresses until our busy is called and we construct the combined
 * command to send to the device.  We can do up to 3 command bytes and 5
 * address bytes.
 */
static void
xlnand_command(device_t self, uint8_t command)
{
	struct xlnand_softc * const xlsc = device_private(self);
	struct xlnand_chip * const xlch = xlsc->xlsc_active_chip;

	KASSERT(xlch != NULL);

	xlch->xlch_cmds |= command << xlch->xlch_cmdshift;
	xlch->xlch_cmdshift += 8;
}

static void
xlnand_address(device_t self, uint8_t address)
{
	struct xlnand_softc * const xlsc = device_private(self);
	struct xlnand_chip * const xlch = xlsc->xlsc_active_chip;

	xlch->xlch_addrs |= address << xlch->xlch_addrshift;
	xlch->xlch_addrshift += 8;
}

static void
xlnand_busy(device_t self)
{
	struct xlnand_softc * const xlsc = device_private(self);
	struct xlnand_chip * const xlch = xlsc->xlsc_active_chip;
	uint32_t last_cmd = xlch->xlch_cmds;
	size_t i;

	xlnand_write_4(xlsc, RMIXLP_NAND_FIFO_INIT, 1);
	xlnand_write_4(xlsc, RMIXLP_NAND_ADDR0_H, xlch->xlch_addrs >> 32);
	xlnand_write_4(xlsc, RMIXLP_NAND_ADDR0_L, xlch->xlch_addrs);
	for (i = 0; i < __arraycount(xlnand_cmdseqs); i++) {
		if (xlch->xlch_cmds == (xlnand_cmdseqs[i] >> 8)) {
			xlnand_write_4(xlsc, RMIXLP_NAND_CMD,
			    xlnand_cmdseqs[i]);
			break;
		}
	}
	KASSERTMSG(i < __arraycount(xlnand_cmdseqs), "%#x", xlch->xlch_cmds);
	xlch->xlch_addrs = 0;
	xlch->xlch_cmds = 0;
	xlch->xlch_datalen = 0;
	xlch->xlch_cmdshift = 0;
	xlch->xlch_addrshift = 0;

	for(u_int count=100000; count--;) {
		uint32_t istatus = xlnand_read_4(xlsc, RMIXLP_NAND_STATUS);
		if ((istatus & RMIXLP_NAND_STATUS_BSY) == 0) {
			if (last_cmd == ONFI_READ_STATUS) {
				xlch->xlch_datalen = 4;
				xlch->xlch_data = xlnand_read_4(xlsc,
				    RMIXLP_NAND_READ_STATUS);
			}
			return;
		}
		DELAY(1);
	}
}

static int
xlnand_intr(void *arg)
{
	struct xlnand_softc * const xlsc = arg;
	int rv = 0;

	mutex_enter(&xlsc->xlsc_intr_lock);

	uint32_t mask = xlnand_read_4(xlsc, RMIXLP_NAND_INT_MASK);

	uint32_t v = xlnand_read_4(xlsc, RMIXLP_NAND_INT_STATUS);
	xlnand_write_4(xlsc, RMIXLP_NAND_INT_MASK, mask & ~v);

	u_int mrdy = __SHIFTOUT(mask & v, RMIXLP_NAND_INT_MRDY);

	struct xlnand_chip *xlch = xlsc->xlsc_chips;
	for (u_int n = ffs(mrdy); n != 0; xlch++, n = ffs(mrdy)) {
		mrdy >>= n;
		xlch += n - 1;
		cv_signal(&xlch->xlch_cv_ready);
		rv = 1;
	}

	mutex_exit(&xlsc->xlsc_intr_lock);
	return rv;
}

static void
xlnand_intr_wait(struct xlnand_softc *xlsc, struct xlnand_chip *xlch)
{
	mutex_enter(&xlsc->xlsc_intr_lock);
	while ((xlnand_read_4(xlsc, RMIXLP_NAND_INT_STATUS) & xlch->xlch_int_mask) == 0) {
		cv_wait(&xlch->xlch_cv_ready, &xlsc->xlsc_intr_lock);
	}
	mutex_exit(&xlsc->xlsc_intr_lock);
}

static void
xlnand_select(device_t self, bool enable)
{
	struct xlnand_softc * const xlsc = device_private(self);
	struct xlnand_chip * const xlch = &xlsc->xlsc_chips[0];

	mutex_enter(&xlsc->xlsc_wait_lock);
	if (enable) {
		if (xlsc->xlsc_active_chip != xlch) {
			while (xlsc->xlsc_active_chip != NULL) {
				cv_wait(&xlsc->xlsc_cv_available,
				    &xlsc->xlsc_wait_lock);
			}
			xlsc->xlsc_active_chip = xlch;
		}
	} else if (xlsc->xlsc_active_chip != NULL) {
		KASSERT(xlsc->xlsc_active_chip == xlch);
		xlsc->xlsc_active_chip = NULL;
		cv_signal(&xlsc->xlsc_cv_available);
	}
	mutex_exit(&xlsc->xlsc_wait_lock);
}

static void
xlnand_read_1(device_t self, uint8_t *p)
{
	struct xlnand_softc * const xlsc = device_private(self);
	struct xlnand_chip * const xlch = xlsc->xlsc_active_chip;
	KASSERT(xlch != NULL);
	if (xlch->xlch_datalen == 0) {
		xlch->xlch_data = xlnand_read_4(xlsc, RMIXLP_NAND_FIFO_DATA);
		xlch->xlch_data = be32toh(xlch->xlch_data);
		xlch->xlch_datalen = 4;
	}
	*p = xlch->xlch_data & 0xff;
	xlch->xlch_data >>= 8;
	xlch->xlch_datalen--;
}

static void
xlnand_read_2(device_t self, uint16_t *p)
{
	struct xlnand_softc * const xlsc = device_private(self);
	struct xlnand_chip * const xlch = xlsc->xlsc_active_chip;
	KASSERT(xlch != NULL);
	if (xlch->xlch_datalen == 0) {
		xlch->xlch_data = xlnand_read_4(xlsc, RMIXLP_NAND_FIFO_DATA);
		xlch->xlch_data = be32toh(xlch->xlch_data);
		xlch->xlch_datalen = 4;
	}
	*p = xlch->xlch_data & 0xffff;
	xlch->xlch_data >>= 16;
	xlch->xlch_datalen -= 2;
}

static void
xlnand_read_buf(device_t self, void *buf, size_t len)
{
	struct xlnand_softc * const xlsc = device_private(self);
	uint32_t *dp32 = (uint32_t *)buf;

	/*
	 * We ignore alignment knowning that XLP will not fault on unaligned
	 * accesses.
	 */
	const size_t len32 = len / 4;
#ifdef __MIPSEB__
	bus_space_read_multi_4(xlsc->xlsc_bst, xlsc->xlsc_bsh,
	    RMIXLP_NAND_FIFO_DATA, dp32, len32);
	dp32 += len32;
#else
	for (size_t i = 0; i < len32; i++) {
		uint32_t v = xlnand_read_4(xlsc, RMIXLP_NAND_FIFO_DATA);
		*dp32++ = be32toh(v);
	}
#endif
	if ((len &= 3) != 0) {
		uint32_t v = xlnand_read_4(xlsc, RMIXLP_NAND_FIFO_DATA);
		v = be32toh(v);
		memcpy(dp32, &v, len);
	}
}

static void
xlnand_write_1(device_t self, uint8_t v)
{
	struct xlnand_softc * const xlsc = device_private(self);

	xlnand_write_4(xlsc, RMIXLP_NAND_FIFO_DATA, htobe32(v));
}

static void
xlnand_write_2(device_t self, uint16_t v)
{
	struct xlnand_softc * const xlsc = device_private(self);

	xlnand_write_4(xlsc, RMIXLP_NAND_FIFO_DATA, htobe32(v));
}

static void
xlnand_write_buf(device_t self, const void *buf, size_t len)
{
	struct xlnand_softc * const xlsc = device_private(self);
	const uint32_t *dp32 = (const uint32_t *)buf;

	const size_t len32 = len / 4;
#ifdef __MIPSEB__
	bus_space_write_multi_4(xlsc->xlsc_bst, xlsc->xlsc_bsh,
	    RMIXLP_NAND_FIFO_DATA, dp32, len32);
	dp32 += len32;
#else
	for (size_t i = 0; i < len32; i++) {
		const uint32_t v = *dp32++;
		xlnand_write_4(xlsc, RMIXLP_NAND_FIFO_DATA, htobe32(v));
	}
#endif

	if ((len &= 3) != 0) {
		uint32_t v = 0;
		memcpy(&v, dp32, len);
		xlnand_write_4(xlsc, RMIXLP_NAND_FIFO_DATA, htobe32(v));
	}
}

static void
xlnand_do_dma(struct xlnand_softc *xlsc, struct xlnand_chip *xlch,
    size_t len, uint64_t address, bool read_p)
{
	bus_dmamap_t map = xlsc->xlsc_xfermap;
	bus_dma_segment_t * const ds = map->dm_segs;

	/*
	 * Step 1
	 */
#if 0
	v = xlnand_read_4(xlsc, RMIXLP_NAND_CTRL);
	v |= RMIXLP_NAND_CTRL_IE;
	xlnand_write_4(xlsc, RMIXLP_NAND_CTRL, v);
#else
	KASSERT(xlnand_read_4(xlsc, RMIXLP_NAND_CTRL) & RMIXLP_NAND_CTRL_IE);
#endif
	/*
	 * Step 2
	 */
	mutex_enter(&xlsc->xlsc_intr_lock);
	xlnand_write_4(xlsc, RMIXLP_NAND_INT_MASK, xlch->xlch_int_mask);
	xlnand_write_4(xlsc, RMIXLP_NAND_INT_STATUS, 0);
	mutex_exit(&xlsc->xlsc_intr_lock);

	/*
	 * Step 3
	 */
	xlnand_write_4(xlsc, RMIXLP_NAND_DMA_CTRL, 
	    RMIXLP_NAND_DMA_CTRL_START
	    | (read_p ? RMIXLP_NAND_DMA_CTRL_READ : RMIXLP_NAND_DMA_CTRL_READ));
	xlnand_write_4(xlsc, RMIXLP_NAND_DMA_BAR_H, ds->ds_addr >> 32);
	xlnand_write_4(xlsc, RMIXLP_NAND_DMA_BAR_L, ds->ds_addr);
	xlnand_write_4(xlsc, RMIXLP_NAND_DMA_CNT, ds->ds_len);

	bus_dmamap_sync(xlsc->xlsc_dmat, map, 0, len,
	    (read_p ? BUS_DMASYNC_PREWRITE : BUS_DMASYNC_PREREAD));

	/*
	 * Step 4.
	 */
	xlnand_write_4(xlsc, RMIXLP_NAND_ADDR0_L, address);
	xlnand_write_4(xlsc, RMIXLP_NAND_ADDR0_H, address >> 32);

	/*
	 * Step 5.
	 */
	xlnand_write_4(xlsc, RMIXLP_NAND_MEMCTRL, xlch->xlch_chipnum);

	/*
	 * Step 6.
	 */
	uint32_t cmd = RMIXLP_NAND_CMD_ISEL | (read_p
	    ? CMDSEQ2(SEQ_10, READ, READ_START)
	    : CMDSEQ2(SEQ_12, PAGE_PROGRAM, PAGE_PROGRAM_START));
	xlnand_write_4(xlsc, RMIXLP_NAND_CMD, cmd);

	xlnand_intr_wait(xlsc, xlch);

	xlnand_write_4(xlsc, RMIXLP_NAND_INT_MASK, RMIXLP_NAND_INT_MRDYn(0));
	xlnand_write_4(xlsc, RMIXLP_NAND_INT_STATUS, 0);


	bus_dmamap_sync(xlsc->xlsc_dmat, map, 0, len,
	    (read_p ? BUS_DMASYNC_POSTWRITE : BUS_DMASYNC_POSTREAD));
}

int
xlnand_read_page(device_t self, size_t offset, uint8_t *data)
{
	struct nand_softc * const sc = device_private(self);
	struct nand_chip * const chip = &sc->sc_chip;
	struct xlnand_softc * const xlsc = device_private(sc->controller_dev);

	KASSERT((offset & (chip->nc_page_size - 1)) == 0);
	uint64_t address = (uint64_t) offset << (8 * chip->nc_addr_cycles_column - chip->nc_page_shift);

	xlnand_select(sc->controller_dev, true);

	xlnand_do_dma(xlsc, xlsc->xlsc_active_chip, chip->nc_page_size,
	    address, true);

	memcpy(data, (void *)xlsc->xlsc_xferbuf, chip->nc_page_size);
	return 0;
}

int
xlnand_program_page(device_t self, size_t offset, const uint8_t *data)
{
	struct nand_softc * const sc = device_private(self);
	struct nand_chip * const chip = &sc->sc_chip;
	struct xlnand_softc * const xlsc = device_private(sc->controller_dev);

	KASSERT((offset & (chip->nc_page_size - 1)) == 0);
	uint64_t address = (uint64_t) offset << (8 * chip->nc_addr_cycles_column - chip->nc_page_shift);

	xlnand_select(sc->controller_dev, true);

	memcpy((void *)xlsc->xlsc_xferbuf, data, chip->nc_page_size);

	xlnand_do_dma(xlsc, xlsc->xlsc_active_chip, chip->nc_page_size,
	    address, false);

	return 0;
}

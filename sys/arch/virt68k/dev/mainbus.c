/*	$NetBSD: mainbus.c,v 1.1 2024/01/02 07:40:59 thorpej Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.1 2024/01/02 07:40:59 thorpej Exp $");

#define _VIRT68K_BUS_DMA_PRIVATE
#define _VIRT68K_BUS_SPACE_PRIVATE

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <machine/bootinfo.h>
#include <machine/cpu.h>

#include <virt68k/dev/mainbusvar.h>

struct virt68k_bus_dma_tag _mainbus_dma_tag = {
	NULL,
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load_direct,
	_bus_dmamap_load_mbuf_direct,
	_bus_dmamap_load_uio_direct,
	_bus_dmamap_load_raw_direct,
	_bus_dmamap_unload,
	NULL,			/* Set up at run-time */
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap
};

struct virt68k_bus_space_tag _mainbus_space_tag = {
	NULL,
	_bus_space_map,
	_bus_space_unmap,
	_bus_space_peek_1,
	_bus_space_peek_2,
	_bus_space_peek_4,
	_bus_space_poke_1,
	_bus_space_poke_2,
	_bus_space_poke_4
};

static int
mainbus_print(void *aux, const char *cp)
{
	struct mainbus_attach_args *ma = aux;

	if (cp) {
		aprint_normal("%s at %s", ma->ma_compatible, cp);
	}

	aprint_normal(" addr 0x%lx", ma->ma_addr);

	return UNCONF;
}

static int
mainbus_match(device_t parent __unused, cfdata_t cf __unused,
    void *args __unused)
{
	static int mainbus_matched;

	if (mainbus_matched)
		return 0;

	return (mainbus_matched = 1);
}

static bool
mainbus_attach_gfpic(struct bi_record *bi, void *v)
{
	struct mainbus_attach_args ma = {
		.ma_st = &_mainbus_space_tag,
		.ma_dmat = &_mainbus_dma_tag,
	};
	device_t self = v;
	struct bi_virt_dev *vd = bootinfo_dataptr(bi);
	int i;

	if (bi->bi_tag == BI_VIRT_GF_PIC_BASE) {
		for (i = 0; i < NPIC; i++) {
			ma.ma_compatible = "google,goldfish-pic";
			ma.ma_addr = vd->vd_mmio_base + (i * 0x1000);
			ma.ma_size = 0x1000;
			ma.ma_irq  = vd->vd_irq_base + i;
			config_found(self, &ma, mainbus_print, CFARGS_NONE);
		}
		return false;	/* done searching */
	}
	return true;		/* keep searching */
}

static bool
mainbus_attach_gfother(struct bi_record *bi, void *v)
{
	struct mainbus_attach_args ma = {
		.ma_st = &_mainbus_space_tag,
		.ma_dmat = &_mainbus_dma_tag,
	};
	device_t self = v;
	struct bi_virt_dev *vd = bootinfo_dataptr(bi);
	int i;

	switch (bi->bi_tag) {
	case BI_VIRT_GF_RTC_BASE:
		/*
		 * There are 2 Goldfish RTC instances on the virt68k
		 * platform:
		 *
		 * 1- This is used as the system timer / hard-clock.
		 * 2- This is used as the TODR.
		 */
		for (i = 0; i < 2; i++) {
			ma.ma_compatible = (i == 0)
			    ? "netbsd,goldfish-rtc-hardclock"
			    : "google,goldfish-rtc";
			ma.ma_addr = vd->vd_mmio_base + (i * 0x1000);
			ma.ma_size = 0x1000;
			ma.ma_irq  = vd->vd_irq_base + i;
			config_found(self, &ma, mainbus_print, CFARGS_NONE);
		}
		break;

	case BI_VIRT_GF_TTY_BASE:
		ma.ma_compatible = "google,goldfish-tty";
		ma.ma_addr = vd->vd_mmio_base;
		ma.ma_size = 0x1000;
		ma.ma_irq  = vd->vd_irq_base;
		config_found(self, &ma, mainbus_print, CFARGS_NONE);
		break;
	}

	return true;		/* keep searching */
}

#define	VIRTIO_MMIO_DEVICE_ID	0x008

static void
mainbus_attach_virtio(device_t self, struct mainbus_attach_args *ma)
{
	bus_space_handle_t bsh;
	uint32_t val;

	/*
	 * Probe the virtio slot to see if there's actually something
	 * there before we claim that it is "found".
	 */
	if (bus_space_map(ma->ma_st, ma->ma_addr, ma->ma_size, 0, &bsh) != 0) {
		aprint_error_dev(self,
		    "unable to map virtio slot @ 0x%lx\n", ma->ma_addr);
		return;
	}
	val = bus_space_read_4(ma->ma_st, bsh, VIRTIO_MMIO_DEVICE_ID);
	bus_space_unmap(ma->ma_st, bsh, ma->ma_size);

	if (val != 0) {
		config_found(self, ma, mainbus_print, CFARGS_NONE);
	}
}

static bool
mainbus_attach_other(struct bi_record *bi, void *v)
{
	struct mainbus_attach_args ma = {
		.ma_st = &_mainbus_space_tag,
		.ma_dmat = &_mainbus_dma_tag,
	};
	device_t self = v;
	struct bi_virt_dev *vd = bootinfo_dataptr(bi);
	int i;

	switch (bi->bi_tag) {
	case BI_VIRT_QEMU_VERSION:
	case BI_VIRT_GF_PIC_BASE:
	case BI_VIRT_GF_RTC_BASE:
	case BI_VIRT_GF_TTY_BASE:
		/* Handled elsewhere. */
		break;

	case BI_VIRT_VIRTIO_BASE:
		for (i = 0; i < (32 * 4); i++) {
			ma.ma_compatible = "virtio,mmio";
			ma.ma_addr = vd->vd_mmio_base + (i * 0x200);
			ma.ma_size = 0x200;
			ma.ma_irq  = vd->vd_irq_base + i;
			mainbus_attach_virtio(self, &ma);
		}
		break;

	case BI_VIRT_CTRL_BASE:
		ma.ma_compatible = "netbsd,qemu-virt-ctrl";	/* XXX */
		ma.ma_addr = vd->vd_mmio_base;
		ma.ma_size = 0x1000;
		ma.ma_irq  = vd->vd_irq_base;
		config_found(self, &ma, mainbus_print, CFARGS_NONE);
		break;

	default:
		if (bi->bi_tag >= BI_MACHDEP(0)) {
			aprint_error_dev(self,
			    "unknown bootinfo tag: 0x%08x\n", bi->bi_tag);
		}
		break;
	}
	return true;		/* keep searching */
}

static void
mainbus_attach(device_t parent __unused, device_t self, void *args __unused)
{

	printf("\n");

	_mainbus_dma_tag._dmamap_sync =
	    (mmutype == MMU_68040) ? _bus_dmamap_sync_0460
				   : _bus_dmamap_sync_030;

	/* Attach the PICs first. */
	bootinfo_enumerate(mainbus_attach_gfpic, self);

	/* Attach the reset of the Goldfish devices next. */
	bootinfo_enumerate(mainbus_attach_gfother, self);

	/* Now the rest. */
	bootinfo_enumerate(mainbus_attach_other, self);
}

int
mainbus_compatible_match(const struct mainbus_attach_args * const ma,
   const struct device_compatible_entry * driver_compats)
{
	const char *device_compats[1] = {
		[0] = ma->ma_compatible,
	};
	return device_compatible_match(device_compats,
	    __arraycount(device_compats), driver_compats);
}

const struct device_compatible_entry *
mainbus_compatible_lookup(const struct mainbus_attach_args * const ma,
    const struct device_compatible_entry * driver_compats)
{
	const char *device_compats[1] = {
		[0] = ma->ma_compatible,
	};
	return device_compatible_lookup(device_compats,
	    __arraycount(device_compats), driver_compats);
}

CFATTACH_DECL_NEW(mainbus, 0,
    mainbus_match, mainbus_attach, NULL, NULL);

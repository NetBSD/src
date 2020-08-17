/*	$NetBSD: mainbus.c,v 1.4 2020/08/17 21:25:12 jmcneill Exp $	*/

/*
 * Copyright (c) 2007
 *      Internet Initiative Japan, Inc.  All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.4 2020/08/17 21:25:12 jmcneill Exp $");

#define	_MIPS_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <mips/cavium/include/mainbusvar.h>
#include <mips/cavium/octeonvar.h>

#include <dev/fdt/fdtvar.h>

static int	mainbus_match(device_t, struct cfdata *, void *);
static void	mainbus_attach(device_t, device_t, void *);
static void	mainbus_attach_static(device_t);
static void	mainbus_attach_devicetree(device_t);
static int	mainbus_submatch(device_t, cfdata_t, const int *, void *);
static int	mainbus_print(void *, const char *);

static void	simplebus_bus_io_init(bus_space_tag_t, void *);

CFATTACH_DECL_NEW(mainbus, sizeof(device_t), mainbus_match, mainbus_attach,
    NULL, NULL);

static struct mips_bus_space simplebus_bus_tag;

static struct mips_bus_dma_tag simplebus_dma_tag = {
	._cookie = NULL,
	._wbase = 0,
	._bounce_alloc_lo = 0,
	._bounce_alloc_hi = 0,
	._dmamap_ops = _BUS_DMAMAP_OPS_INITIALIZER,
	._dmamem_ops = _BUS_DMAMEM_OPS_INITIALIZER,
	._dmatag_ops = _BUS_DMATAG_OPS_INITIALIZER,
};

static int
mainbus_match(device_t parent, struct cfdata *match, void *aux)
{
	static int once = 0;

	if (once != 0)
		return 0;
	once = 1;

	return 1;
}

static void
mainbus_attach(device_t parent, device_t self, void *aux)
{
	aprint_normal("\n");

	if (fdtbus_get_data() != NULL) {
		mainbus_attach_devicetree(self);
	} else {
		mainbus_attach_static(self);
	}
}

static void
mainbus_attach_static(device_t self)
{
	struct mainbus_attach_args aa;
	int i;

	for (i = 0; i < (int)mainbus_ndevs; i++) {
		aa.aa_name = mainbus_devs[i];
		config_found_sm_loc(self, "mainbus", NULL, &aa,
		    mainbus_print, mainbus_submatch);
	}
}

extern struct octeon_config octeon_configuration;
extern void octpow_bootstrap(struct octeon_config *);
extern void octfpa_bootstrap(struct octeon_config *);

static void
mainbus_attach_devicetree(device_t self)
{
	const struct fdt_console *cons = fdtbus_get_console();
	struct mainbus_attach_args aa;
	struct fdt_attach_args faa;
	u_int uart_freq;

	aa.aa_name = "cpunode";
	config_found_sm_loc(self, "mainbus", NULL, &aa, mainbus_print,
	    mainbus_submatch);

	aa.aa_name = "iobus";
	config_found_sm_loc(self, "mainbus", NULL, &aa, mainbus_print,
	    mainbus_submatch);

	octpow_bootstrap(&octeon_configuration);
	octfpa_bootstrap(&octeon_configuration);

	simplebus_bus_io_init(&simplebus_bus_tag, NULL);

	faa.faa_bst = &simplebus_bus_tag;
	faa.faa_a4x_bst = NULL;		/* XXX */
	faa.faa_dmat = &simplebus_dma_tag;
	faa.faa_name = "";

	if (cons != NULL) {
		faa.faa_phandle = fdtbus_get_stdout_phandle();

		if (of_getprop_uint32(faa.faa_phandle, "clock-frequency",
		    &uart_freq) != 0) {
			uart_freq = octeon_ioclock_speed();
		}

		if (uart_freq > 0)
			delay(640000000 / uart_freq);

		cons->consinit(&faa, uart_freq);
	}

	faa.faa_phandle = OF_peer(0);
	config_found(self, &faa, NULL);
}

static int
mainbus_submatch(device_t parent, cfdata_t cf, const int *locs, void *aux)
{
	struct mainbus_attach_args *aa = aux;

	if (strcmp(cf->cf_name, aa->aa_name) != 0)
		return 0;

	return config_match(parent, cf, aux);
}

static int
mainbus_print(void *aux, const char *pnp)
{
	struct mainbus_attach_args *aa = aux;

	if (pnp != 0)
		return QUIET;

	if (pnp)
		aprint_normal("%s at %s", aa->aa_name, pnp);

	return UNCONF;
}

/* ---- bus_space(9) */
#define	CHIP			simplebus
#define	CHIP_IO
#define	CHIP_ACCESS_SIZE	8

#define	CHIP_W1_BUS_START(v)	0x0000000000000000ULL
#define	CHIP_W1_BUS_END(v)	0x7fffffffffffffffULL
#define	CHIP_W1_SYS_START(v)	0x8000000000000000ULL
#define	CHIP_W1_SYS_END(v)	0xffffffffffffffffULL

#include <mips/mips/bus_space_alignstride_chipdep.c>

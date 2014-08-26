/*	$NetBSD: exynos_soc.c,v 1.15 2014/08/26 11:55:54 reinoud Exp $	*/
/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk.
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

#include "opt_exynos.h"

#define	_ARM32_BUS_DMA_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: exynos_soc.c,v 1.15 2014/08/26 11:55:54 reinoud Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <prop/proplib.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <arm/locore.h>

#include <arm/mainbus/mainbus.h>
#include <arm/cortex/mpcore_var.h>

#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_var.h>
#include <arm/samsung/mct_reg.h>
#include <arm/samsung/smc.h>

#include <arm/cortex/pl310_var.h>
#include <arm/cortex/pl310_reg.h>

/* XXXNH */
#include <evbarm/odroid/platform.h>

bus_space_handle_t exynos_core_bsh;
bus_space_handle_t exynos_audiocore_bsh;

/* these variables are retrieved in start.S and stored in .data */
uint32_t  exynos_soc_id = 0;
uint32_t  exynos_pop_id = 0;


/*
 * the early serial console
 */
#ifdef EXYNOS_CONSOLE_EARLY

#include "opt_sscom.h"
#include <arm/samsung/sscom_reg.h>
#include <arm/samsung/sscom_var.h>
#include <dev/cons.h>

static volatile uint8_t *uart_base;

#define CON_REG(a) (*((volatile uint32_t *)(uart_base + (a))))

static int
exynos_cngetc(dev_t dv)
{
        if ((CON_REG(SSCOM_UTRSTAT) & UTRSTAT_RXREADY) == 0)
		return -1;

	return CON_REG(SSCOM_URXH);
}

static void
exynos_cnputc(dev_t dv, int c)
{
	int timo = 150000;

	while ((CON_REG(SSCOM_UFSTAT) & UFSTAT_TXFULL) && --timo > 0);

	CON_REG(SSCOM_UTXH) = c & 0xff;
}

static struct consdev exynos_earlycons = {
	.cn_putc = exynos_cnputc,
	.cn_getc = exynos_cngetc,
	.cn_pollc = nullcnpollc,
};
#endif /* EXYNOS_CONSOLE_EARLY */


#ifdef ARM_TRUSTZONE_FIRMWARE
int
exynos_do_idle(void)
{
        exynos_smc(SMC_CMD_SLEEP, 0, 0, 0);

	return 0;
}


int
exynos_set_cpu_boot_addr(int cpu, vaddr_t boot_addr)
{
	/* XXX we need to map in iRAM space for this XXX */
	return 0;
}


int
exynos_cpu_boot(int cpu)
{
	exynos_smc(SMC_CMD_CPU1BOOT, cpu, 0, 0);

	return 0;
}


/*
 * The latency values used below are `magic' and probably chosen empiricaly.
 * For the 4210 variant the data latency is lower, a 0x110. This is currently
 * not enforced.
 *
 * The prefetch values are also different for the revision 0 of the
 * Exynos4412, but why?
 */

int
exynos_l2cc_init(void)
{
	const uint32_t tag_latency  = 0x110;
	const uint32_t data_latency = IS_EXYNOS4410_P() ? 0x110 : 0x120;
	const uint32_t prefetch4412   = /* 0111 0001 0000 0000 0000 0000 0000 0111 */
				PREFETCHCTL_DBLLINEF_EN  |
				PREFETCHCTL_INSTRPREF_EN |
				PREFETCHCTL_DATAPREF_EN  |
				PREFETCHCTL_PREF_DROP_EN |
				PREFETCHCTL_PREFETCH_OFFSET_7;
	const uint32_t prefetch4412_r0 = /* 0011 0000 0000 0000 0000 0000 0000 0111 */
				PREFETCHCTL_INSTRPREF_EN |
				PREFETCHCTL_DATAPREF_EN  |
				PREFETCHCTL_PREFETCH_OFFSET_7;
	const uint32_t aux_val      =    /* 0111 1100 0100 0111 0000 0000 0000 0001 */
				AUXCTL_EARLY_BRESP_EN |
				AUXCTL_I_PREFETCH     |
				AUXCTL_D_PREFETCH     |
				AUXCTL_NS_INT_ACC_CTL |
				AUXCTL_NS_INT_LOCK_EN |
				AUXCTL_SHARED_ATT_OVR |
				AUXCTL_WAY_SIZE_RSVD7 << 16 | /* why rsvd7 ??? */
				AUXCTL_FULL_LINE_WR0;
	const uint32_t aux_keepmask =    /* 1100 0010 0000 0000 1111 1111 1111 1111  */
				AUXCTL_RSVD31         |
				AUXCTL_EARLY_BRESP_EN |
				AUXCTL_CACHE_REPL_RR  |

				AUXCTL_SH_ATTR_INV_ENA|
				AUXCTL_EXCL_CACHE_CFG |
				AUXCTL_ST_BUF_DEV_LIM_EN |
				AUXCTL_HIPRO_SO_DEV_EN |
				AUXCTL_FULL_LINE_WR0  |
				0xffff;
	uint32_t prefetch;

	/* check the bitmaps are the same as the linux implementation uses */
	KASSERT(prefetch4412    == 0x71000007);
	KASSERT(prefetch4412_r0 == 0x30000007);
	KASSERT(aux_val         == 0x7C470001);
	KASSERT(aux_keepmask    == 0xC200FFFF);

	if (IS_EXYNOS4412_R0_P())
		prefetch = prefetch4412_r0;
	else
		prefetch = prefetch4412;	/* newer than >= r1_0 */
	;

	exynos_smc(SMC_CMD_L2X0SETUP1, tag_latency, data_latency, prefetch);
	exynos_smc(SMC_CMD_L2X0SETUP2,
		POWERCTL_DYNCLKGATE | POWERCTL_STANDBY,
		aux_val, aux_keepmask);
	exynos_smc(SMC_CMD_L2X0INVALL, 0, 0, 0);
	exynos_smc(SMC_CMD_L2X0CTRL, 1, 0, 0);

	return 0;
}
#endif /* ARM_TRUSTZONE_FIRMWARE */


void
exynos_bootstrap(vaddr_t iobase, vaddr_t uartbase)
{
	int error;
	size_t core_size, audiocore_size;
	size_t audiocore_pbase, audiocore_vbase __diagused;

#ifdef EXYNOS4
	if (IS_EXYNOS4_P()) {
		core_size = EXYNOS4_CORE_SIZE;
		audiocore_size = EXYNOS4_AUDIOCORE_SIZE;
		audiocore_pbase = EXYNOS4_AUDIOCORE_PBASE;
		audiocore_vbase = EXYNOS4_AUDIOCORE_VBASE;
	}
#endif

#ifdef EXYNOS5
	if (IS_EXYNOS5_P()) {
		core_size = EXYNOS5_CORE_SIZE;
		audiocore_size = EXYNOS5_AUDIOCORE_SIZE;
		audiocore_pbase = EXYNOS5_AUDIOCORE_PBASE;
		audiocore_vbase = EXYNOS5_AUDIOCORE_VBASE;
	}
#endif

	/* set up early console so we can use printf() and friends */
#ifdef EXYNOS_CONSOLE_EARLY
	uart_base = (volatile uint8_t *) uartbase;
	cn_tab = &exynos_earlycons;
	printf("Exynos early console operational\n\n");
#endif
	/* map in the exynos io registers */
	error = bus_space_map(&exynos_bs_tag, EXYNOS_CORE_PBASE,
		core_size, 0, &exynos_core_bsh);
	if (error)
		panic("%s: failed to map in Exynos SFR registers: %d",
			__func__, error);
	KASSERT(exynos_core_bsh == iobase);

	error = bus_space_map(&exynos_bs_tag, audiocore_pbase,
		audiocore_size, 0, &exynos_audiocore_bsh);
	if (error)
		panic("%s: failed to map in Exynos audio SFR registers: %d",
			__func__, error);
	KASSERT(exynos_audiocore_bsh == audiocore_vbase);

	/* init bus dma tags */
	exynos_dma_bootstrap(physmem * PAGE_SIZE);

	/* gpio bootstrapping delayed */
}


void
exynos_device_register(device_t self, void *aux)
{
	if (device_is_a(self, "armperiph")
	    && device_is_a(device_parent(self), "mainbus")) {
		/*
		 * XXX KLUDGE ALERT XXX
		 * The iot mainbus supplies is completely wrong since it scales
		 * addresses by 2.  The simpliest remedy is to replace with our
		 * bus space used for the armcore regisers (which armperiph uses).
		 */
		struct mainbus_attach_args * const mb = aux;
		mb->mb_iot = &exynos_bs_tag;
		return;
	}
	if (device_is_a(self, "armgic")
	    && device_is_a(device_parent(self), "armperiph")) {
		/*
		 * The Exynos4420 armgic is located at a different location!
		 */

		extern uint32_t exynos_soc_id;

		switch (EXYNOS_PRODUCT_ID(exynos_soc_id)) {
#if defined(EXYNOS5)
		case 0xe5410:
			/* offsets not changed on matt's request */
#if 0
			mpcaa->mpcaa_memh = EXYNOS_CORE_VBASE;
			mpcaa->mpcaa_off1 = EXYNOS5_GIC_IOP_DISTRIBUTOR_OFFSET;
			mpcaa->mpcaa_off2 = EXYNOS5_GIC_IOP_CONTROLLER_OFFSET;
#endif
			break;
#endif
#if defined(EXYNOS4)
		case 0xe4410:
		case 0xe4412: {
			struct mpcore_attach_args * const mpcaa = aux;

			mpcaa->mpcaa_memh = EXYNOS_CORE_VBASE;
			mpcaa->mpcaa_off1 = EXYNOS4_GIC_DISTRIBUTOR_OFFSET;
			mpcaa->mpcaa_off2 = EXYNOS4_GIC_CNTR_OFFSET;
			break;
		      }
#endif
		default:
			panic("%s: unknown SoC product id %#x", __func__,
			    (u_int)EXYNOS_PRODUCT_ID(exynos_soc_id));
		}
		return;
	}
	if (device_is_a(self, "armgtmr") || device_is_a(self, "mct")) {
#ifdef EXYNOS5
		/*
		 * The global timer is dependent on the MCT running.
		 */
		bus_size_t o = EXYNOS5_MCT_OFFSET + MCT_G_TCON;
		uint32_t v = bus_space_read_4(&exynos_bs_tag, exynos_core_bsh,
		     o);
		v |= G_TCON_START;
		bus_space_write_4(&exynos_bs_tag, exynos_core_bsh, o, v);
#endif
		/*
		 * The frequencies of the timers are the reference
		 * frequency.
		 */
		prop_dictionary_set_uint32(device_properties(self),
		    "frequency", EXYNOS_F_IN_FREQ);
		return;
	}

	exyo_device_register(self, aux);
}


void
exynos_device_register_post_config(device_t self, void *aux)
{
	exyo_device_register_post_config(self, aux);
}


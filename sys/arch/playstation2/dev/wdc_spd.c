/*	$NetBSD: wdc_spd.c,v 1.19 2005/06/26 19:54:42 he Exp $	*/

/*-
 * Copyright (c) 2001, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wdc_spd.c,v 1.19 2005/06/26 19:54:42 he Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#define __read_1(a)							\
({									\
	u_int32_t ra_ = (a);						\
	u_int8_t r = (*(__volatile__ u_int8_t *)ra_);			\
									\
	if (ra_ == 0xb400004e)	/* (wdc)STAT  LED off */		\
		SPD_LED_OFF();						\
									\
	(r);								\
})
#define __write_1(a, v)							\
{									\
	u_int32_t wa_ = (a);						\
	(*(__volatile__ u_int8_t *)wa_) = (v);				\
									\
	if (wa_ == 0xb400004e)	/* (wdc)CMD  LED on */			\
		SPD_LED_ON();						\
}
#define _PLAYSTATION2_BUS_SPACE_PRIVATE
#include <machine/bus.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

#include <playstation2/ee/eevar.h>
#include <playstation2/dev/spdvar.h>
#include <playstation2/dev/spdreg.h>

#define	WDC_SPD_HDD_AUXREG_OFFSET		0x1c

struct wdc_spd_softc {
	struct wdc_softc sc_wdcdev;
	struct ata_channel *sc_chanlist[1];
	struct ata_channel sc_channel;
	struct ata_queue sc_chqueue;
	struct wdc_regs sc_wdc_regs;
	void *sc_ih;
};

#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif

STATIC int wdc_spd_match(struct device *, struct cfdata *, void *);
STATIC void wdc_spd_attach(struct device *, struct device *, void *);

CFATTACH_DECL(wdc_spd, sizeof (struct wdc_spd_softc),
    wdc_spd_match, wdc_spd_attach, NULL, NULL);

extern struct cfdriver wdc_cd;

STATIC void __wdc_spd_enable(void);
STATIC void __wdc_spd_disable(void) __attribute__((__unused__));
STATIC void __wdc_spd_bus_space(struct ata_channel *);

/*
 * wdc register is 16 bit wide.
 */
#define VADDR(h, o)	((h) + (o))
_BUS_SPACE_READ(_wdc_spd, 1, 8)
_BUS_SPACE_READ(_wdc_spd, 2, 16)
_BUS_SPACE_READ_MULTI(_wdc_spd, 1, 8)
_BUS_SPACE_READ_MULTI(_wdc_spd, 2, 16)
_BUS_SPACE_READ_REGION(_wdc_spd, 1, 8)
_BUS_SPACE_READ_REGION(_wdc_spd, 2, 16)
_BUS_SPACE_WRITE(_wdc_spd, 1, 8)
_BUS_SPACE_WRITE(_wdc_spd, 2, 16)
_BUS_SPACE_WRITE_MULTI(_wdc_spd, 1, 8)
_BUS_SPACE_WRITE_MULTI(_wdc_spd, 2, 16)
_BUS_SPACE_WRITE_REGION(_wdc_spd, 1, 8)
_BUS_SPACE_WRITE_REGION(_wdc_spd, 2, 16)
_BUS_SPACE_SET_MULTI(_wdc_spd, 1, 8)
_BUS_SPACE_SET_MULTI(_wdc_spd, 2, 16)
_BUS_SPACE_SET_REGION(_wdc_spd, 1, 8)
_BUS_SPACE_SET_REGION(_wdc_spd, 2, 16)
_BUS_SPACE_COPY_REGION(_wdc_spd, 1, 8)
_BUS_SPACE_COPY_REGION(_wdc_spd, 2, 16)
#undef VADDR

STATIC const struct playstation2_bus_space _wdc_spd_space = {
	pbs_map		: _BUS_SPACE_NO_MAP,
	pbs_unmap	: _BUS_SPACE_NO_UNMAP,
	pbs_subregion	: _BUS_SPACE_NO_SUBREGION,
	pbs_alloc	: _BUS_SPACE_NO_ALLOC,
	pbs_free	: _BUS_SPACE_NO_FREE,
	pbs_vaddr	: _BUS_SPACE_NO_VADDR,
	pbs_r_1		: _wdc_spd_read_1,
	pbs_r_2		: _wdc_spd_read_2,
	pbs_r_4		: _BUS_SPACE_NO_READ(4, 32),
	pbs_r_8		: _BUS_SPACE_NO_READ(8, 64),
	pbs_rm_1	: _wdc_spd_read_multi_1,
	pbs_rm_2	: _wdc_spd_read_multi_2,
	pbs_rm_4	: _BUS_SPACE_NO_READ_MULTI(4, 32),
	pbs_rm_8	: _BUS_SPACE_NO_READ_MULTI(8, 64),
	pbs_rr_1	: _wdc_spd_read_region_1,
	pbs_rr_2	: _wdc_spd_read_region_2,
	pbs_rr_4	: _BUS_SPACE_NO_READ_REGION(4, 32),
	pbs_rr_8	: _BUS_SPACE_NO_READ_REGION(8, 64),
	pbs_w_1		: _wdc_spd_write_1,
	pbs_w_2		: _wdc_spd_write_2,
	pbs_w_4		: _BUS_SPACE_NO_WRITE(4, 32),
	pbs_w_8		: _BUS_SPACE_NO_WRITE(8, 64),
	pbs_wm_1	: _wdc_spd_write_multi_1,
	pbs_wm_2	: _wdc_spd_write_multi_2,
	pbs_wm_4	: _BUS_SPACE_NO_WRITE_MULTI(4, 32),
	pbs_wm_8	: _BUS_SPACE_NO_WRITE_MULTI(8, 64),
	pbs_wr_1	: _wdc_spd_write_region_1,
	pbs_wr_2	: _wdc_spd_write_region_2,
	pbs_wr_4	: _BUS_SPACE_NO_WRITE_REGION(4, 32),
	pbs_wr_8	: _BUS_SPACE_NO_WRITE_REGION(8, 64),
	pbs_sm_1	: _wdc_spd_set_multi_1,
	pbs_sm_2	: _wdc_spd_set_multi_2,
	pbs_sm_4	: _BUS_SPACE_NO_SET_MULTI(4, 32),
	pbs_sm_8	: _BUS_SPACE_NO_SET_MULTI(8, 64),
	pbs_sr_1	: _wdc_spd_set_region_1,
	pbs_sr_2	: _wdc_spd_set_region_2,
	pbs_sr_4	: _BUS_SPACE_NO_SET_REGION(4, 32),
	pbs_sr_8	: _BUS_SPACE_NO_SET_REGION(8, 64),
	pbs_c_1		: _wdc_spd_copy_region_1,
	pbs_c_2		: _wdc_spd_copy_region_2,
	pbs_c_4		: _BUS_SPACE_NO_COPY_REGION(4, 32),
	pbs_c_8		: _BUS_SPACE_NO_COPY_REGION(8, 64),
};

int
wdc_spd_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct spd_attach_args *spa = aux;
	struct ata_channel ch;
	struct wdc_softc wdc;
	struct wdc_regs wdr;
	int i, result;

	if (spa->spa_slot != SPD_HDD)
		return (0);

	memset(&wdc, 0, sizeof(wdc));
	memset(&ch, 0, sizeof(ch));
	ch.ch_atac = &wdc.sc_atac;
	wdc.regs = &wdr;

	__wdc_spd_bus_space(&ch);

	for (i = 0, result = 0; i < 8; i++) { /* 8 sec */
		if (result == 0)
			result = wdcprobe(&ch);
		delay(1000000);
	}
	
	return (result);
}

void
wdc_spd_attach(struct device *parent, struct device *self, void *aux)
{
	struct spd_attach_args *spa = aux;
	struct wdc_spd_softc *sc = (void *)self;
	struct wdc_softc *wdc = &sc->sc_wdcdev;
	struct ata_channel *ch = &sc->sc_channel;

	printf(": %s\n", spa->spa_product_name);

	sc->sc_wdcdev.regs = &sc->sc_wdc_regs;

	wdc->sc_atac.atac_cap =
	    ATAC_CAP_DMA | ATAC_CAP_UDMA | ATAC_CAP_DATA16;
	wdc->sc_atac.atac_pio_cap = 0;
	sc->sc_chanlist[0] = &sc->sc_channel;
	wdc->sc_atac.atac_channels = sc->sc_chanlist;
	wdc->sc_atac.atac_nchannels = 1;
	ch->ch_channel = 0;
	ch->ch_atac = &sc->sc_wdcdev.sc_atac;
	ch->ch_queue = &sc->sc_chqueue;

	__wdc_spd_bus_space(ch);

	spd_intr_establish(SPD_HDD, wdcintr, &sc->sc_channel);

	__wdc_spd_enable();

	wdcattach(&sc->sc_channel);
}

void
__wdc_spd_bus_space(struct ata_channel *ch)
{
	struct wdc_regs *wdr = CHAN_TO_WDC_REGS(ch);
	int i;

	wdr->cmd_iot = &_wdc_spd_space;
	for (i = 0; i < 8; i++)
		wdr->cmd_iohs[i] = SPD_HDD_IO_BASE + i * 2; /*  wdc register is 16 bit wide. */
	wdc_init_shadow_regs(ch);
	wdr->ctl_iot = &_wdc_spd_space;
	wdr->ctl_ioh = SPD_HDD_IO_BASE + WDC_SPD_HDD_AUXREG_OFFSET;
	wdr->data32iot = wdr->cmd_iot;
	wdr->data32ioh = SPD_HDD_IO_BASE;
}

void
__wdc_spd_enable()
{
	u_int16_t r;

	r = _reg_read_2(SPD_INTR_ENABLE_REG16);
	r |= SPD_INTR_HDD;
	_reg_write_2(SPD_INTR_ENABLE_REG16, r);
}

void
__wdc_spd_disable()
{
	u_int16_t r;

	r = _reg_read_2(SPD_INTR_ENABLE_REG16);
	r &= ~SPD_INTR_HDD;
	_reg_write_2(SPD_INTR_ENABLE_REG16, r);
}

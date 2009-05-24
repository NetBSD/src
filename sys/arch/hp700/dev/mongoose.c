/*	$NetBSD: mongoose.c,v 1.15 2009/05/24 06:53:34 skrll Exp $	*/

/*	$OpenBSD: mongoose.c,v 1.7 2000/08/15 19:42:56 mickey Exp $	*/

/*
 * Copyright (c) 1998,1999 Michael Shalayeff
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mongoose.c,v 1.15 2009/05/24 06:53:34 skrll Exp $");

#define MONGOOSE_DEBUG 9

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/bus.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <hp700/hp700/intr.h>
#include <hp700/dev/cpudevs.h>
#include <hp700/dev/viper.h>

#include <dev/eisa/eisareg.h>
#include <dev/eisa/eisavar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

/* EISA Bus Adapter registers definitions */
#define	MONGOOSE_MONGOOSE	0x10000
struct mongoose_regs {
	uint8_t	version;
	uint8_t	lock;
	uint8_t	liowait;
	uint8_t	clock;
	uint8_t	reserved[0xf000 - 4];
	uint8_t	intack;
};

#define	MONGOOSE_CTRL		0x00000
#define	MONGOOSE_NINTS		16
struct mongoose_ctrl {
	struct dma0 {
		struct {
			uint32_t	addr : 8;
			uint32_t	count: 8;
		} ch[4];
		uint8_t	command;
		uint8_t	request;
		uint8_t	mask_channel;
		uint8_t	mode;
		uint8_t	clr_byte_ptr;
		uint8_t	master_clear;
		uint8_t	mask_clear;
		uint8_t	master_write;
		uint8_t	pad[8];
	}	dma0;

	uint8_t	irr0;		/* 0x20 */
	uint8_t	imr0;
	uint8_t	iack;		/* 0x22 -- 2 b2b reads generate
					(e)isa Iack cycle & returns int level */
	uint8_t	pad0[29];

	struct timers {
		uint8_t	sysclk;
		uint8_t	refresh;
		uint8_t	spkr;
		uint8_t	ctrl;
		uint32_t	pad;
	}	tmr[2];			/* 0x40 -- timers control */
	uint8_t	pad1[16];

	uint16_t	inmi;		/* 0x60 NMI control */
	uint8_t	pad2[30];
	struct {
		uint8_t	pad0;
		uint8_t	ch2;
		uint8_t	ch3;
		uint8_t	ch1;
		uint8_t	pad1;
		uint8_t	pad2[3];
		uint8_t	ch0;
		uint8_t	pad4;
		uint8_t	ch6;
		uint8_t	ch7;
		uint8_t	ch5;
		uint8_t	pad5[3];
		uint8_t	pad6[16];
	} pr;				/* 0x80 */

	uint8_t	irr1;		/* 0xa0 */
	uint8_t	imr1;
	uint8_t	pad3[30];

	struct dma1 {
		struct {
			uint32_t	addr : 8;
			uint32_t	pad0 : 8;
			uint32_t	count: 8;
			uint32_t	pad1 : 8;
		} ch[4];
		uint8_t	command;
		uint8_t	pad0;
		uint8_t	request;
		uint8_t	pad1;
		uint8_t	mask_channel;
		uint8_t	pad2;
		uint8_t	mode;
		uint8_t	pad3;
		uint8_t	clr_byte_ptr;
		uint8_t	pad4;
		uint8_t	master_clear;
		uint8_t	pad5;
		uint8_t	mask_clear;
		uint8_t	pad6;
		uint8_t	master_write;
		uint8_t	pad7;
	}	dma1;			/* 0xc0 */

	uint8_t	master_req;	/* 0xe0 master request register */
	uint8_t	pad4[31];

	uint8_t	pad5[0x3d0];	/* 0x4d0 */
	uint8_t	pic0;		/* 0 - edge, 1 - level */
	uint8_t	pic1;
	uint8_t	pad6[0x460];
	uint8_t	nmi;
	uint8_t	nmi_ext;
#define	MONGOOSE_NMI_BUSRESET	0x01
#define	MONGOOSE_NMI_IOPORT_EN	0x02
#define	MONGOOSE_NMI_EN		0x04
#define	MONGOOSE_NMI_MTMO_EN	0x08
#define	MONGOOSE_NMI_RES4	0x10
#define	MONGOOSE_NMI_IOPORT_INT	0x20
#define	MONGOOSE_NMI_MASTER_INT	0x40
#define	MONGOOSE_NMI_INT	0x80
};

#define	MONGOOSE_IOMAP	0x100000

struct hppa_isa_iv {
	int (*iv_handler)(void *arg);
	void *iv_arg;
	int iv_pri;

	struct evcnt iv_evcnt;
	/* don't do sharing, we won't have many slots anyway
	struct hppa_isa_iv *iv_next;
	*/
};

struct mongoose_softc {
	device_t sc_dev;
	void *sc_ih;

	bus_space_tag_t sc_bt;
	volatile struct mongoose_regs *sc_regs;
	volatile struct mongoose_ctrl *sc_ctrl;
	bus_addr_t sc_iomap;

	/* interrupts section */
	struct hppa_eisa_chipset sc_ec;
	struct hppa_isa_chipset sc_ic;
	struct hppa_isa_iv sc_iv[MONGOOSE_NINTS];

	/* isa/eisa bus guts */
	struct hppa_bus_space_tag sc_eiot;
	struct hppa_bus_space_tag sc_ememt;
	struct hppa_bus_dma_tag sc_edmat;
	struct hppa_bus_space_tag sc_iiot;
	struct hppa_bus_space_tag sc_imemt;
	struct hppa_bus_dma_tag sc_idmat;
};

union mongoose_attach_args {
	struct eisabus_attach_args mongoose_eisa;
	struct isabus_attach_args mongoose_isa;
};

void mg_eisa_attach_hook(device_t, device_t, struct eisabus_attach_args *);
int mg_intr_map(void *, u_int, eisa_intr_handle_t *);
const char *mg_intr_string(void *, int);
void mg_isa_attach_hook(device_t, device_t, struct isabus_attach_args *);
void *mg_intr_establish(void *, int, int, int, int (*)(void *), void *);
void mg_intr_disestablish(void *, void *);
int mg_intr_check(void *, int, int);
int mg_intr(void *);
int mg_eisa_iomap(void *, bus_addr_t, bus_size_t, int, bus_space_handle_t *);
int mg_eisa_memmap(void *, bus_addr_t, bus_size_t, int, bus_space_handle_t *);
void mg_eisa_memunmap(void *, bus_space_handle_t, bus_size_t);
void mg_isa_barrier(void *, bus_space_handle_t, bus_size_t, bus_size_t, int);
uint16_t mg_isa_r2(void *, bus_space_handle_t, bus_size_t);
uint32_t mg_isa_r4(void *, bus_space_handle_t, bus_size_t);
void mg_isa_w2(void *, bus_space_handle_t, bus_size_t, uint16_t);
void mg_isa_w4(void *, bus_space_handle_t, bus_size_t, uint32_t);
void mg_isa_rm_2(void *, bus_space_handle_t, bus_size_t, uint16_t *, bus_size_t);
void mg_isa_rm_4(void *, bus_space_handle_t, bus_size_t, uint32_t *, bus_size_t);
void mg_isa_wm_2(void *, bus_space_handle_t, bus_size_t, const uint16_t *, bus_size_t);
void mg_isa_wm_4(void *, bus_space_handle_t, bus_size_t, const uint32_t *, bus_size_t);
void mg_isa_sm_2(void *, bus_space_handle_t, bus_size_t, uint16_t, bus_size_t);
void mg_isa_sm_4(void *, bus_space_handle_t, bus_size_t, uint32_t, bus_size_t);
void mg_isa_rr_2(void *, bus_space_handle_t, bus_size_t, uint16_t *, bus_size_t);
void mg_isa_rr_4(void *, bus_space_handle_t, bus_size_t, uint32_t *, bus_size_t);
void mg_isa_wr_2(void *, bus_space_handle_t, bus_size_t, const uint16_t *, bus_size_t);
void mg_isa_wr_4(void *, bus_space_handle_t, bus_size_t, const uint32_t *, bus_size_t);
void mg_isa_sr_2(void *, bus_space_handle_t, bus_size_t, uint16_t, bus_size_t);
void mg_isa_sr_4(void *, bus_space_handle_t, bus_size_t, uint32_t, bus_size_t);

int	mgmatch(device_t, cfdata_t, void *);
void	mgattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mongoose, sizeof(struct mongoose_softc),
    mgmatch, mgattach, NULL, NULL);

/* TODO: DMA guts */

void
mg_eisa_attach_hook(device_t parent, device_t self,
	struct eisabus_attach_args *mg)
{
}

int
mg_intr_map(void *v, u_int irq, eisa_intr_handle_t *ehp)
{
	*ehp = irq;
	return 0;
}

const char *
mg_intr_string(void *v, int irq)
{
	static char buf[16];

	sprintf (buf, "isa irq %d", irq);
	return buf;
}

void
mg_isa_attach_hook(device_t parent, device_t self,
	struct isabus_attach_args *iba)
{

}

void *
mg_intr_establish(void *v, int irq, int type, int pri,
	int (*handler)(void *), void *arg)
{
	struct hppa_isa_iv *iv;
	struct mongoose_softc *sc = v;
	volatile uint8_t *imr, *pic;

	if (!sc || irq < 0 || irq >= MONGOOSE_NINTS ||
	    (0 <= irq && irq < MONGOOSE_NINTS && sc->sc_iv[irq].iv_handler))
		return NULL;

	if (type != IST_LEVEL && type != IST_EDGE) {
		aprint_debug_dev(sc->sc_dev, "bad interrupt level (%d)\n",
		    type);
		return NULL;
	}

	iv = &sc->sc_iv[irq];
	if (iv->iv_handler) {
		aprint_debug_dev(sc->sc_dev, "irq %d already established\n",
		    irq);
		return NULL;
	}

	iv->iv_pri = pri;
	iv->iv_handler = handler;
	iv->iv_arg = arg;
	
	if (irq < 8) {
		imr = &sc->sc_ctrl->imr0;
		pic = &sc->sc_ctrl->pic0;
	} else {
		imr = &sc->sc_ctrl->imr1;
		pic = &sc->sc_ctrl->pic1;
		irq -= 8;
	}

	*imr |= 1 << irq;
	*pic |= (type == IST_LEVEL) << irq;

	/* TODO: ack it? */

	return iv;
}

void
mg_intr_disestablish(void *v, void *cookie)
{
	struct hppa_isa_iv *iv = cookie;
	struct mongoose_softc *sc = v;
 	int irq = iv - sc->sc_iv;
 	volatile uint8_t *imr;

	if (!sc || !cookie)
		return;

	if (irq < 8)
		imr = &sc->sc_ctrl->imr0;
	else
		imr = &sc->sc_ctrl->imr1;
	*imr &= ~(1 << irq);
	/* TODO: ack it? */

	iv->iv_handler = NULL;
}

int
mg_intr_check(void *v, int irq, int type)
{
	return 0;
}

int
mg_intr(void *v)
{
	struct mongoose_softc *sc = v;
	struct hppa_isa_iv *iv;
	int s, irq = 0;

	iv = &sc->sc_iv[irq];
	s = splraise(imask[iv->iv_pri]);
	(iv->iv_handler)(iv->iv_arg);
	splx(s);

	return 0;
}

int
mg_eisa_iomap(void *v, bus_addr_t addr, bus_size_t size, int cacheable,
	bus_space_handle_t *bshp)
{
	struct mongoose_softc *sc = v;

	/* see if it's ISA space we are mapping */
	if (0x100 <= addr && addr < 0x400) {
#define	TOISA(a) ((((a) & 0x3f8) << 9) + ((a) & 7))
		size = TOISA(addr + size) - TOISA(addr);
		addr = TOISA(addr);
	}

	return (sc->sc_bt->hbt_map)(NULL, sc->sc_iomap + addr, size,
				    cacheable, bshp);
}

int
mg_eisa_memmap(void *v, bus_addr_t addr, bus_size_t size, int cacheable,
	bus_space_handle_t *bshp)
{
	/* TODO: eisa memory map */
	return -1;
}

void
mg_eisa_memunmap(void *v, bus_space_handle_t bsh, bus_size_t size)
{
	/* TODO: eisa memory unmap */
}

void
mg_isa_barrier(void *v, bus_space_handle_t h, bus_size_t o, bus_size_t l, int op)
{
	sync_caches();
}

uint16_t
mg_isa_r2(void *v, bus_space_handle_t h, bus_size_t o)
{
	uint16_t r = *((volatile uint16_t *)(h + o));

	return le16toh(r);
}

uint32_t
mg_isa_r4(void *v, bus_space_handle_t h, bus_size_t o)
{
	uint32_t r = *((volatile uint32_t *)(h + o));

	return le32toh(r);
}

void
mg_isa_w2(void *v, bus_space_handle_t h, bus_size_t o, uint16_t vv)
{
	*((volatile uint16_t *)(h + o)) = htole16(vv);
}

void
mg_isa_w4(void *v, bus_space_handle_t h, bus_size_t o, uint32_t vv)
{
	*((volatile uint32_t *)(h + o)) = htole32(vv);
}

void
mg_isa_rm_2(void *v, bus_space_handle_t h, bus_size_t o, uint16_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(a++) = le16toh(*(volatile uint16_t *)h);
}

void
mg_isa_rm_4(void *v, bus_space_handle_t h, bus_size_t o, uint32_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(a++) = le32toh(*(volatile uint32_t *)h);
}

void
mg_isa_wm_2(void *v, bus_space_handle_t h, bus_size_t o, const uint16_t *a, bus_size_t c)
{
	uint16_t r;

	h += o;
	while (c--) {
		r = *(a++);
		*(volatile uint16_t *)h = htole16(r);
	}
}

void
mg_isa_wm_4(void *v, bus_space_handle_t h, bus_size_t o, const uint32_t *a, bus_size_t c)
{
	uint32_t r;

	h += o;
	while (c--) {
		r = *(a++);
		*(volatile uint32_t *)h = htole32(r);
	}
}

void
mg_isa_sm_2(void *v, bus_space_handle_t h, bus_size_t o, uint16_t vv, bus_size_t c)
{
	vv = htole16(vv);
	h += o;
	while (c--)
		*(volatile uint16_t *)h = vv;
}

void
mg_isa_sm_4(void *v, bus_space_handle_t h, bus_size_t o, uint32_t vv, bus_size_t c)
{
	vv = htole32(vv);
	h += o;
	while (c--)
		*(volatile uint32_t *)h = vv;
}

void
mg_isa_rr_2(void *v, bus_space_handle_t h, bus_size_t o, uint16_t *a, bus_size_t c)
{
	uint16_t r;
	volatile uint16_t *p;

	h += o;
	p = (void *)h;
	while (c--) {
		r = *p++;
		*a++ = le16toh(r);
	}
}

void
mg_isa_rr_4(void *v, bus_space_handle_t h, bus_size_t o, uint32_t *a, bus_size_t c)
{
	uint32_t r;
	volatile uint32_t *p;

	h += o;
	p = (void *)h;
	while (c--) {
		r = *p++;
		*a++ = le32toh(r);
	}
}

void
mg_isa_wr_2(void *v, bus_space_handle_t h, bus_size_t o, const uint16_t *a, bus_size_t c)
{
	uint16_t r;
	volatile uint16_t *p;

	h += o;
	p = (void *)h;
	while (c--) {
		r = *a++;
		*p++ = htole16(r);
	}
}

void
mg_isa_wr_4(void *v, bus_space_handle_t h, bus_size_t o, const uint32_t *a, bus_size_t c)
{
	uint32_t r;
	volatile uint32_t *p;

	h += o;
	p = (void *)h;
	while (c--) {
		r = *a++;
		*p++ = htole32(r);
	}
}

void
mg_isa_sr_2(void *v, bus_space_handle_t h, bus_size_t o, uint16_t vv, bus_size_t c)
{
	volatile uint16_t *p;

	vv = htole16(vv);
	h += o;
	p = (void *)h;
	while (c--)
		*p++ = vv;
}

void
mg_isa_sr_4(void *v, bus_space_handle_t h, bus_size_t o, uint32_t vv, bus_size_t c)
{
	volatile uint32_t *p;

	vv = htole32(vv);
	h += o;
	p = (void *)h;
	while (c--)
		*p++ = vv;
}

int
mgmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;
	bus_space_handle_t ioh;

	if (ca->ca_type.iodc_type != HPPA_TYPE_BHA ||
	    ca->ca_type.iodc_sv_model != HPPA_BHA_EISA)
		return 0;

	if (bus_space_map(ca->ca_iot, ca->ca_hpa + MONGOOSE_MONGOOSE,
			  sizeof(struct mongoose_regs), 0, &ioh))
		return 0;

	/* XXX check EISA signature */

	bus_space_unmap(ca->ca_iot, ioh, sizeof(struct mongoose_regs));

	return 1;
}

void
mgattach(device_t parent, device_t self, void *aux)
{
	struct confargs *ca = aux;
	struct mongoose_softc *sc = device_private(self);
	struct hppa_bus_space_tag *bt;
	union mongoose_attach_args ea;
	char brid[EISA_IDSTRINGLEN];
	bus_space_handle_t ioh;

	sc->sc_dev = self;
	sc->sc_bt = ca->ca_iot;
	sc->sc_iomap = ca->ca_hpa;
	if (bus_space_map(ca->ca_iot, ca->ca_hpa + MONGOOSE_MONGOOSE,
			  sizeof(struct mongoose_regs), 0, &ioh))
		panic("mgattach: can't map registers");
	sc->sc_regs = (struct mongoose_regs *)ioh;
	if (bus_space_map(ca->ca_iot, ca->ca_hpa + MONGOOSE_CTRL,
			  sizeof(struct mongoose_ctrl), 0, &ioh))
		panic("mgattach: can't map control registers");
	sc->sc_ctrl = (struct mongoose_ctrl *)ioh;

	viper_eisa_en();

	/* BUS RESET */
	sc->sc_ctrl->nmi_ext = MONGOOSE_NMI_BUSRESET;
	DELAY(1);
	sc->sc_ctrl->nmi_ext = 0;
	DELAY(100);

	/* determine eisa board id */
	{
		uint8_t id[4], *p;
		/* XXX this is awful */
		p = (uint8_t *)(ioh + EISA_SLOTOFF_VID);
		id[0] = *p++;
		id[1] = *p++;
		id[2] = *p++;
		id[3] = *p++;

		brid[0] = EISA_VENDID_0(id);
		brid[1] = EISA_VENDID_1(id);
		brid[2] = EISA_VENDID_2(id);
		brid[3] = EISA_PRODID_0(id + 2);
		brid[4] = EISA_PRODID_1(id + 2);
		brid[5] = EISA_PRODID_2(id + 2);
		brid[6] = EISA_PRODID_3(id + 2);
		brid[7] = '\0';
	}

	aprint_normal(": %s rev %d, %d MHz\n", brid, sc->sc_regs->version,
	    (sc->sc_regs->clock? 33 : 25));
	sc->sc_regs->liowait = 1;	/* disable isa wait states */
	sc->sc_regs->lock    = 1;	/* bus unlock */

	/* attach EISA */
	sc->sc_ec.ec_v = sc;
	sc->sc_ec.ec_attach_hook = mg_eisa_attach_hook;
	sc->sc_ec.ec_intr_establish = mg_intr_establish;
	sc->sc_ec.ec_intr_disestablish = mg_intr_disestablish;
	sc->sc_ec.ec_intr_string = mg_intr_string;
	sc->sc_ec.ec_intr_map = mg_intr_map;

	/* inherit the bus tags for eisa from the mainbus */
	bt = &sc->sc_eiot;
	memcpy(bt, ca->ca_iot, sizeof(*bt));
	bt->hbt_cookie = sc;
	bt->hbt_map = mg_eisa_iomap;
#define	R(n)	bt->__CONCAT(hbt_,n) = &__CONCAT(mg_isa_,n)
	/* R(barrier); */
	R(r2); R(r4); R(w2); R(w4);
	R(rm_2);R(rm_4);R(wm_2);R(wm_4);R(sm_2);R(sm_4);
	R(rr_2);R(rr_4);R(wr_2);R(wr_4);R(sr_2);R(sr_4);

	bt = &sc->sc_ememt;
	memcpy(bt, ca->ca_iot, sizeof(*bt));
	bt->hbt_cookie = sc;
	bt->hbt_map = mg_eisa_memmap;
	bt->hbt_unmap = mg_eisa_memunmap;

	/* attachment guts */
	ea.mongoose_eisa.eba_iot = &sc->sc_eiot;
	ea.mongoose_eisa.eba_memt = &sc->sc_ememt;
	ea.mongoose_eisa.eba_dmat = NULL /* &sc->sc_edmat */;
	ea.mongoose_eisa.eba_ec = &sc->sc_ec;
	config_found_ia(self, "eisabus", &ea.mongoose_eisa, eisabusprint);

	sc->sc_ic.ic_v = sc;
	sc->sc_ic.ic_attach_hook = mg_isa_attach_hook;
	sc->sc_ic.ic_intr_establish = mg_intr_establish;
	sc->sc_ic.ic_intr_disestablish = mg_intr_disestablish;
	sc->sc_ic.ic_intr_check = mg_intr_check;

	/* inherit the bus tags for isa from the eisa */
	bt = &sc->sc_imemt;
	memcpy(bt, &sc->sc_ememt, sizeof(*bt));
	bt = &sc->sc_iiot;
	memcpy(bt, &sc->sc_eiot, sizeof(*bt));

	/* TODO: DMA tags */

	/* attachment guts */
	ea.mongoose_isa.iba_iot = &sc->sc_iiot;
	ea.mongoose_isa.iba_memt = &sc->sc_imemt;
#if NISADMA > 0
	ea.mongoose_isa.iba_dmat = &sc->sc_idmat;
#endif
	ea.mongoose_isa.iba_ic = &sc->sc_ic;
	config_found_ia(self, "isabus", &ea.mongoose_isa, isabusprint);
#undef	R

	/* attach interrupt */
	sc->sc_ih = hp700_intr_establish(sc->sc_dev, IPL_NONE, mg_intr, sc,
	    &int_reg_cpu, ca->ca_irq);
}

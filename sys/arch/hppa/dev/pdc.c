/*	$NetBSD: pdc.c,v 1.2 2014/07/26 20:24:33 dholland Exp $	*/

/*	$OpenBSD: pdc.c,v 1.14 2001/04/29 21:05:43 mickey Exp $	*/

/*
 * Copyright (c) 1998-2003 Michael Shalayeff
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
__KERNEL_RCSID(0, "$NetBSD: pdc.c,v 1.2 2014/07/26 20:24:33 dholland Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/callout.h>
#include <sys/conf.h>
#include <sys/kauth.h>

#include <dev/cons.h>
#include <dev/clock_subr.h>

#include <machine/pdc.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <hppa/hppa/machdep.h>
#include <hppa/dev/cpudevs.h>

typedef
struct pdc_softc {
	device_t sc_dv;
	struct tty *sc_tty;
	struct callout sc_to;
} pdcsoftc_t;

pdcio_t pdc;

enum pdc_type pdc_type;

static struct pdc_result pdcret1 PDC_ALIGNMENT;
static struct pdc_result pdcret2 PDC_ALIGNMENT;
static char pdc_consbuf[IODC_MINIOSIZ] PDC_ALIGNMENT;

iodcio_t pdc_cniodc, pdc_kbdiodc;
pz_device_t *pz_kbd, *pz_cons;

int pdcmatch(device_t, cfdata_t, void *);
void pdcattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(pdc, sizeof(pdcsoftc_t),
    pdcmatch, pdcattach, NULL, NULL);

extern struct cfdriver pdc_cd;

static int pdc_attached;

dev_type_open(pdcopen);
dev_type_close(pdcclose);
dev_type_read(pdcread);
dev_type_write(pdcwrite);
dev_type_ioctl(pdcioctl);
dev_type_stop(pdcstop);
dev_type_tty(pdctty);
dev_type_poll(pdcpoll);

const struct cdevsw pdc_cdevsw = {
	.d_open = pdcopen,
	.d_close = pdcclose,
	.d_read = pdcread,
	.d_write = pdcwrite,
	.d_ioctl = pdcioctl,
	.d_stop = pdcstop,
	.d_tty = pdctty,
	.d_poll = pdcpoll,
	.d_mmap = nommap,
	.d_kpqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

void pdcstart(struct tty *);
void pdctimeout(void *);
int pdcparam(struct tty *, struct termios *);
int pdccnlookc(dev_t, int *);

static struct cnm_state pdc_cnm_state;

static int pdcgettod(todr_chip_handle_t, struct timeval *);
static int pdcsettod(todr_chip_handle_t, struct timeval *);

void
pdc_init(void)
{
	static int kbd_iodc[IODC_MAXSIZE/sizeof(int)];
	static int cn_iodc[IODC_MAXSIZE/sizeof(int)];
	int err;
	int pagezero_cookie;

	pagezero_cookie = hppa_pagezero_map();

	pz_kbd = &PAGE0->mem_kbd;
	pz_cons = &PAGE0->mem_cons;

	pdc = (pdcio_t)PAGE0->mem_pdc;

	/* XXX should we reset the console/kbd here?
	   well, /boot did that for us anyway */
	if ((err = pdc_call((iodcio_t)pdc, 0, PDC_IODC, PDC_IODC_READ,
	      &pdcret1, pz_cons->pz_hpa, IODC_IO, cn_iodc, IODC_MAXSIZE)) < 0 ||
	    (err = pdc_call((iodcio_t)pdc, 0, PDC_IODC, PDC_IODC_READ,
	      &pdcret1, pz_kbd->pz_hpa, IODC_IO, kbd_iodc, IODC_MAXSIZE)) < 0) {
#ifdef DEBUG
		printf("pdc_init: failed reading IODC (%d)\n", err);
#endif
	}

	hppa_pagezero_unmap(pagezero_cookie);

	pdc_cniodc = (iodcio_t)cn_iodc;
	pdc_kbdiodc = (iodcio_t)kbd_iodc;

	/* XXX make pdc current console */
	cn_tab = &constab[0];
}

void
pdc_settype(int modelno)
{
	switch (modelno) {
		/* 720, 750, 730, 735, 755 */
	case HPPA_BOARD_HP720:
	case HPPA_BOARD_HP750_66:
	case HPPA_BOARD_HP730_66:
	case HPPA_BOARD_HP735_99:
	case HPPA_BOARD_HP755_99:
	case HPPA_BOARD_HP755_125:
	case HPPA_BOARD_HP735_130:

		/* 710, 705, 7[12]5 */
	case HPPA_BOARD_HP710:
	case HPPA_BOARD_HP705:
	case HPPA_BOARD_HP715_50:
	case HPPA_BOARD_HP715_33:
	case HPPA_BOARD_HP715S_50:
	case HPPA_BOARD_HP715S_33:
	case HPPA_BOARD_HP715T_50:
	case HPPA_BOARD_HP715T_33:
	case HPPA_BOARD_HP715_75:
	case HPPA_BOARD_HP715_99:
	case HPPA_BOARD_HP725_50:
	case HPPA_BOARD_HP725_75:
	case HPPA_BOARD_HP725_99:

		/* 745, 742, 747 */
	case HPPA_BOARD_HP745I_50:
	case HPPA_BOARD_HP742I_50:
	case HPPA_BOARD_HP747I_100:

		/* 712/{60,80,100,120}, 715/{64,80,100,...}, etc */
	case HPPA_BOARD_HP712_60:
	case HPPA_BOARD_HP712_80:
	case HPPA_BOARD_HP712_100:
	case HPPA_BOARD_HP743I_64:
	case HPPA_BOARD_HP743I_100:
	case HPPA_BOARD_HP712_120:
	case HPPA_BOARD_HP715_80:
	case HPPA_BOARD_HP715_64:
	case HPPA_BOARD_HP715_100:
	case HPPA_BOARD_HP715_100XC:
	case HPPA_BOARD_HP725_100:
	case HPPA_BOARD_HP725_120:
	case HPPA_BOARD_HP715_100L:
	case HPPA_BOARD_HP715_120L:
	case HPPA_BOARD_HP725_80L:
	case HPPA_BOARD_HP725_100L:
	case HPPA_BOARD_HP725_120L:
	case HPPA_BOARD_HP743_50:
	case HPPA_BOARD_HP743_100:
	case HPPA_BOARD_HP715_80M:
	case HPPA_BOARD_HP811:
	case HPPA_BOARD_HP801:
	case HPPA_BOARD_HP743T:
		pdc_type = PDC_TYPE_SNAKE;
		break;

	default:
		pdc_type = PDC_TYPE_UNKNOWN;
	}
}

enum pdc_type
pdc_gettype(void)
{

	return pdc_type;
}

int
pdcmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	/* there could be only one */
	if (pdc_attached || strcmp(ca->ca_name, "pdc"))
		return 0;

	return 1;
}

void
pdcattach(device_t parent, device_t self, void *aux)
{
	static struct todr_chip_handle todr = {
		.todr_settime = pdcsettod,
		.todr_gettime = pdcgettod,
	};
	struct pdc_softc *sc = device_private(self);

	sc->sc_dv = self;
	pdc_attached = 1;

	KASSERT(pdc != NULL);

	cn_init_magic(&pdc_cnm_state);
	cn_set_magic("+++++");

	/* attach the TOD clock */
	todr_attach(&todr);

	aprint_normal("\n");

	callout_init(&sc->sc_to, 0);
}

int
pdcopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct pdc_softc *sc;
	struct tty *tp;
	int s;
	int error = 0, setuptimeout;

	sc = device_lookup_private(&pdc_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;

	s = spltty();

	if (sc->sc_tty) {
		tp = sc->sc_tty;
	} else {
		tp = tty_alloc();
		sc->sc_tty = tp;
		tty_attach(tp);
	}

	tp->t_oproc = pdcstart;
	tp->t_param = pdcparam;
	tp->t_dev = dev;

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp)) {
		splx(s);
		return (EBUSY);
	}

	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_state |= TS_CARR_ON;
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG|CLOCAL;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = 9600;
		ttsetwater(tp);

		setuptimeout = 1;
	} else
		setuptimeout = 0;
	tp->t_state |= TS_CARR_ON;

	splx(s);

	error = (*tp->t_linesw->l_open)(dev, tp);
	if (error == 0 && setuptimeout)
		pdctimeout(sc);

	return error;
}

int
pdcclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct tty *tp;
	struct pdc_softc *sc;

	sc = device_lookup_private(&pdc_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;

	tp = sc->sc_tty;
	callout_stop(&sc->sc_to);
	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);
	return 0;
}

int
pdcread(dev_t dev, struct uio *uio, int flag)
{
	struct tty *tp;
	struct pdc_softc *sc;

	sc = device_lookup_private(&pdc_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;

	tp = sc->sc_tty;
	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
pdcwrite(dev_t dev, struct uio *uio, int flag)
{
	struct tty *tp;
	struct pdc_softc *sc;

	sc = device_lookup_private(&pdc_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;

	tp = sc->sc_tty;
	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
pdcpoll(dev_t dev, int events, struct lwp *l)
{
	struct pdc_softc *sc = device_lookup_private(&pdc_cd,minor(dev));
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

int
pdcioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	int error;
	struct tty *tp;
	struct pdc_softc *sc;

	sc = device_lookup_private(&pdc_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;

	tp = sc->sc_tty;
	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error >= 0)
		return error;
	error = ttioctl(tp, cmd, data, flag, l);
	if (error >= 0)
		return error;

	return ENOTTY;
}

int
pdcparam(struct tty *tp, struct termios *t)
{

	return 0;
}

void
pdcstart(struct tty *tp)
{
	int s;

	s = spltty();
	if (tp->t_state & (TS_TTSTOP | TS_BUSY)) {
		splx(s);
		return;
	}
	ttypull(tp);
	tp->t_state |= TS_BUSY;
	while (tp->t_outq.c_cc != 0)
		pdccnputc(tp->t_dev, getc(&tp->t_outq));
	tp->t_state &= ~TS_BUSY;
	splx(s);
}

void
pdcstop(struct tty *tp,	int flag)
{
	int s;

	s = spltty();
	if (tp->t_state & TS_BUSY)
		if ((tp->t_state & TS_TTSTOP) == 0)
			tp->t_state |= TS_FLUSH;
	splx(s);
}

void
pdctimeout(void *v)
{
	struct pdc_softc *sc = v;
	struct tty *tp = sc->sc_tty;
	int c;

	while (pdccnlookc(tp->t_dev, &c)) {
		cn_check_magic(tp->t_dev, c, pdc_cnm_state);
		if (tp->t_state & TS_ISOPEN)
			(*tp->t_linesw->l_rint)(c, tp);
	}
	callout_reset(&sc->sc_to, 1, pdctimeout, sc);
}

struct tty *
pdctty(dev_t dev)
{
	struct pdc_softc *sc;

	sc = device_lookup_private(&pdc_cd, minor(dev));
	if (sc == NULL)
		return NULL;

	return sc->sc_tty;
}

void
pdccnprobe(struct consdev *cn)
{

	cn->cn_dev = makedev(22,0);
	cn->cn_pri = CN_NORMAL;
}

void
pdccninit(struct consdev *cn)
{
#ifdef DEBUG
	printf("pdc0: console init\n");
#endif
}

int
pdccnlookc(dev_t dev, int *cp)
{
	int s, err __debugused, l, pagezero_cookie;

	s = splhigh();
	pagezero_cookie = hppa_pagezero_map();
	err = pdc_call(pdc_kbdiodc, 0, pz_kbd->pz_hpa, IODC_IO_CONSIN,
	    pz_kbd->pz_spa, pz_kbd->pz_layers, &pdcret1, 0, pdc_consbuf, 1, 0);
	l = pdcret1.result[0];
	*cp = pdc_consbuf[0];
	hppa_pagezero_unmap(pagezero_cookie);
	splx(s);

#ifdef DEBUG
	if (err < 0)
		printf("pdccnlookc: input error: %d\n", err);
#endif
	return l;
}

int
pdccngetc(dev_t dev)
{
	int c;

	if (!pdc)
		return 0;
	while (!pdccnlookc(dev, &c))
		;
	return (c);
}

void
pdccnputc(dev_t dev, int c)
{
	int s, err, pagezero_cookie;

	s = splhigh();
	pagezero_cookie = hppa_pagezero_map();
	*pdc_consbuf = c;
	err = pdc_call(pdc_cniodc, 0, pz_cons->pz_hpa, IODC_IO_CONSOUT,
	    pz_cons->pz_spa, pz_cons->pz_layers, &pdcret1, 0, pdc_consbuf, 1, 0);
	hppa_pagezero_unmap(pagezero_cookie);
	splx(s);

	if (err < 0) {
#if defined(DDB) || defined(KGDB)
		Debugger();
#endif /* DDB || KGDB */
		delay(250000);
#if 0
		/*
		 * It's not a good idea to use the output to print
		 * an output error.
		 */
		printf("pdccnputc: output error: %d\n", err);
#endif
	}
}

void
pdccnpollc(dev_t dev, int on)
{
}

static int
pdcgettod(todr_chip_handle_t tch, struct timeval *tvp)
{
	struct pdc_tod *tod = (struct pdc_tod *)&pdcret1;
	int error;

	error = pdc_call((iodcio_t)pdc, 1, PDC_TOD, PDC_TOD_READ,
	    &pdcret1);

	if (error == 0) {
		tvp->tv_sec = tod->sec;
		tvp->tv_usec = tod->usec;
	}
	return error;
}

static int
pdcsettod(todr_chip_handle_t tch, struct timeval *tvp)
{
	int error;

	error = pdc_call((iodcio_t)pdc, 1, PDC_TOD, PDC_TOD_WRITE,
	    tvp->tv_sec, tvp->tv_usec);

	return error;
}


int
pdcproc_chassis_display(unsigned long disp)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_CHASSIS, PDC_CHASSIS_DISP, disp);
	
	return err;
}

int
pdcproc_chassis_info(struct pdc_chassis_info *pci, struct pdc_chassis_lcd *pcl)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_CHASSIS, PDC_CHASSIS_INFO,
	    &pdcret1, &pdcret2 , sizeof(*pcl));
	if (err < 0)
		return err;

	memcpy(pci, &pdcret1, sizeof(*pci));
	memcpy(pcl, &pdcret2, sizeof(*pcl));
	
	return err;
}

int
pdcproc_pim(int type, struct pdc_pim *pp, void **buf, size_t *sz)
{
	static char data[896] __attribute__((__aligned__(8)));
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_PIM, type, &pdcret1, data,
	    sizeof(data));
	if (err < 0)
		return err;

	memcpy(pp, &pdcret1, sizeof(*pp));
	*buf = data;
	*sz = sizeof(data);

	return err;
}

int
pdcproc_model_info(struct pdc_model *pm)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_MODEL, PDC_MODEL_INFO, &pdcret1);
	if (err < 0)
		return err;

	memcpy(pm, &pdcret1, sizeof(*pm));

	return err;
}

int
pdcproc_model_cpuid(struct pdc_cpuid *pc)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_MODEL, PDC_MODEL_CPUID, &pdcret1);
	if (err < 0)
		return err;

	memcpy(pc, &pdcret1, sizeof(*pc));

	return err;
}

int
pdcproc_cache(struct pdc_cache *pc)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_CACHE, PDC_CACHE_DFLT, &pdcret1);

	if (err < 0)
		return err;

	memcpy(pc, &pdcret1, sizeof(*pc));

	return err;
}


int
pdcproc_cache_spidbits(struct pdc_spidb *pcs)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_CACHE, PDC_CACHE_GETSPIDB,
            &pdcret1);

	if (err < 0)
		return err;

	memcpy(pcs, &pdcret1, sizeof(*pcs));

	return err;
}

int
pdcproc_hpa_processor(hppa_hpa_t *hpa)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_HPA, PDC_HPA_DFLT, &pdcret1);
	if (err < 0)
		return err;

	*hpa = pdcret1.result[0];

	return err;
}

int
pdcproc_coproc(struct pdc_coproc *pc)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_COPROC, PDC_COPROC_DFLT, &pdcret1);
	if (err < 0)
		return err;

	memcpy(pc, &pdcret1, sizeof(*pc));

	return err;
}

int
pdcproc_iodc_read(hppa_hpa_t hpa, int command, int *actcnt,
    struct pdc_iodc_read *buf1, size_t sz1, struct iodc_data *buf2,
    size_t sz2)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_IODC, PDC_IODC_READ,
	    &pdcret1, hpa, command, &pdcret2, sizeof(pdcret2));

	if (err < 0)
		return err;

	if (actcnt != NULL) {
		struct pdc_iodc_read *pir = (struct pdc_iodc_read *)&pdcret1;

		*actcnt = pir->size;
	}

	memcpy(buf1, &pdcret1, sz1);
	memcpy(buf2, &pdcret2, sz2);

	return err;
}

int
pdcproc_iodc_ninit(struct pdc_iodc_minit *pimi, hppa_hpa_t hpa, int sz)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_IODC, PDC_IODC_NINIT, &pdcret1,
	    hpa, sz);

	if (err < 0)
		return err;

	memcpy(pimi, &pdcret1, sizeof(*pimi));

	return err;
}

int
pdcproc_instr(unsigned int *mem)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_INSTR, PDC_INSTR_DFLT, &pdcret1);
	if (err < 0)
		return err;

	memcpy(mem, &pdcret1, sizeof(*mem));

	return err;
}
 
int
pdcproc_block_tlb(struct pdc_btlb *pb)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_BLOCK_TLB, PDC_BTLB_DEFAULT,
	    &pdcret1);
	if (err < 0)
		return err;

	memcpy(pb, &pdcret1, sizeof(*pb));

	return err;
}

int
pdcproc_btlb_insert(pa_space_t sp, vaddr_t va, paddr_t pa, vsize_t sz,
    u_int prot, int index)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_BLOCK_TLB, PDC_BTLB_INSERT, sp,
	    va, pa, sz, prot, index);

	return err;
}

int
pdcproc_btlb_purge(pa_space_t sp, vaddr_t va, paddr_t pa, vsize_t sz)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_BLOCK_TLB, PDC_BTLB_PURGE, sp, va,
	    pa, sz);

	return err;
}

int
pdcproc_btlb_purgeall(void)
{
	int err;

	err =  pdc_call((iodcio_t)pdc, 0, PDC_BLOCK_TLB, PDC_BTLB_PURGE_ALL);

	return err;
}

int pdcproc_tlb_info(struct pdc_hwtlb *ph)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_TLB, PDC_TLB_INFO, &pdcret1);
	if (err < 0)
		return err;

	memcpy(ph, &pdcret1, sizeof(*ph));

	return err;
}

int
pdcproc_tlb_config(struct pdc_hwtlb *ph, unsigned long hpt,
    unsigned long hptsize, unsigned long type)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_TLB, PDC_TLB_CONFIG, ph, hpt,
	    hptsize, type);

	return err;
}

int
pdcproc_system_map_find_mod(struct pdc_system_map_find_mod *psm,
    struct device_path *dev, int mod)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_SYSTEM_MAP,
	    PDC_SYSTEM_MAP_FIND_MOD, &pdcret1, &pdcret2, mod);
	if (err < 0)
		return err;

	memcpy(psm, &pdcret1, sizeof(*psm));
	memcpy(dev, &pdcret2, sizeof(*dev));

	return err;
}

int
pdcproc_system_map_find_addr(struct pdc_system_map_find_addr *psm, int mod,
    int addr)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_SYSTEM_MAP,
	    PDC_SYSTEM_MAP_FIND_ADDR, &pdcret1, mod, addr);
	if (err < 0)
		return err;

	memcpy(psm, &pdcret1, sizeof(*psm));

	return err;
	
}

int
pdcproc_system_map_trans_path(struct pdc_memmap *pmm, struct device_path *dev)
{
	int err;

	memcpy(&pdcret2, dev, sizeof(*dev));

	err = pdc_call((iodcio_t)pdc, 0, PDC_SYSTEM_MAP,
	    PDC_SYSTEM_MAP_TRANS_PATH, &pdcret1, &pdcret2);
	if (err < 0)
		return err;

	memcpy(pmm, &pdcret1, sizeof(*pmm));

	return err;
}

int
pdcproc_soft_power_info(struct pdc_power_info *pspi)
{
	int err;
	
	err = pdc_call((iodcio_t)pdc, 0, PDC_SOFT_POWER, PDC_SOFT_POWER_INFO,
	    &pdcret1, 0);
	if (err < 0)
		return err;

	memcpy(pspi, &pdcret1, sizeof(*pspi));

	return err;
}

int
pdcproc_soft_power_enable(int action)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_SOFT_POWER, PDC_SOFT_POWER_ENABLE,
	    &pdcret1, action);

	return err;
}

int
pdcproc_memmap(struct pdc_memmap *pmm, struct device_path *dev)
{
	int err;

	memcpy(&pdcret2, dev, sizeof(*dev));

	err = pdc_call((iodcio_t)pdc, 0, PDC_MEMMAP, PDC_MEMMAP_HPA, &pdcret1,
	    &pdcret2);
	if (err < 0)
		return err;

	memcpy(pmm, &pdcret1, sizeof(*pmm));

	return err;
}

int
pdcproc_ioclrerrors(void)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_IO, PDC_IO_READ_AND_CLEAR_ERRORS);

	return err;
}

int
pdcproc_ioreset(void)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_IO, PDC_IO_RESET_DEVICES);

	return err;
}

int
pdcproc_doreset(void)
{
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_BROADCAST_RESET, PDC_DO_RESET);

	return err;
}

int
pdcproc_lan_station_id(char *addr, size_t sz, hppa_hpa_t hpa)
{
	struct pdc_lan_station_id *mac = (struct pdc_lan_station_id *)&pdcret1;
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_LAN_STATION_ID,
	    PDC_LAN_STATION_ID_READ, &pdcret1, hpa);
	if (err < 0)
		return err;

	memcpy(addr, mac->addr, sz);

	return 0;
}

int
pdcproc_pci_inttblsz(int *nentries)
{
	struct pdc_pat_io_num *ppio = (struct pdc_pat_io_num *)&pdcret1;
	int err;

	err = pdc_call((iodcio_t)pdc, 0, PDC_PCI_INDEX, PDC_PCI_GET_INT_TBL_SZ,
	    &pdcret1);

	*nentries = ppio->num;

	return err;
}

/* Maximum number of supported interrupt routing entries. */
#define MAX_INT_TBL_SZ	16

int
pdcproc_pci_gettable(int nentries, size_t size, void *table)
{
	int err;
	static struct pdc_pat_pci_rt int_tbl[MAX_INT_TBL_SZ] PDC_ALIGNMENT;

	if (nentries > MAX_INT_TBL_SZ)
		panic("interrupt routing table too big (%d entries)", nentries);

	pdcret1.result[0] = nentries;

	err = pdc_call((iodcio_t)pdc, 0, PDC_PCI_INDEX, PDC_PCI_GET_INT_TBL,
	    &pdcret1, 0, &int_tbl);
	if (err < 0)
		return err;
	    
	memcpy(table, int_tbl, size);

	return err;
}

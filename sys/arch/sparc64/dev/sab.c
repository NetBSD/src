/*	$NetBSD: sab.c,v 1.52 2014/07/25 08:10:35 dholland Exp $	*/
/*	$OpenBSD: sab.c,v 1.7 2002/04/08 17:49:42 jason Exp $	*/

/*
 * Copyright (c) 2001 Jason L. Wright (jason@thought.net)
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
 *	This product includes software developed by Jason L. Wright
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Effort sponsored in part by the Defense Advanced Research Projects
 * Agency (DARPA) and Air Force Research Laboratory, Air Force
 * Materiel Command, USAF, under agreement number F30602-01-2-0537.
 *
 */

/*
 * SAB82532 Dual UART driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sab.c,v 1.52 2014/07/25 08:10:35 dholland Exp $");

#include "opt_kgdb.h"
#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/syslog.h>
#include <sys/kauth.h>
#include <sys/kgdb.h>
#include <sys/intr.h>

#include <machine/autoconf.h>
#include <machine/openfirm.h>

#include <dev/cons.h>

#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>
#include <sparc64/dev/sab82532reg.h>

#include "locators.h"

#define SABUNIT(x)		(minor(x) & 0x7ffff)
#define SABDIALOUT(x)		(minor(x) & 0x80000)

#define	SABTTY_RBUF_SIZE	1024	/* must be divisible by 2 */

struct sab_softc {
	device_t		sc_dev;
	struct intrhand *	sc_ih;
	bus_space_tag_t		sc_bt;
	bus_space_handle_t	sc_bh;
	struct sabtty_softc *	sc_child[SAB_NCHAN];
	u_int			sc_nchild;
	void *			sc_softintr;
	int			sc_node;
};

struct sabtty_attach_args {
	u_int sbt_portno;
};

struct sabtty_softc {
	device_t		sc_dev;
	struct sab_softc *	sc_parent;
	bus_space_tag_t		sc_bt;
	bus_space_handle_t	sc_bh;
	struct tty *		sc_tty;
	u_int			sc_portno;
	uint8_t			sc_pvr_dtr, sc_pvr_dsr;
	uint8_t			sc_imr0, sc_imr1;
	int			sc_openflags;
	u_char *		sc_txp;
	int			sc_txc;
	int			sc_flags;
#define SABTTYF_STOP		0x0001
#define	SABTTYF_DONE		0x0002
#define	SABTTYF_RINGOVERFLOW	0x0004
#define	SABTTYF_CDCHG		0x0008
#define	SABTTYF_CONS_IN		0x0010
#define	SABTTYF_CONS_OUT	0x0020
#define	SABTTYF_TXDRAIN		0x0040
#define	SABTTYF_DONTDDB		0x0080
#define SABTTYF_IS_RSC		0x0100
	uint8_t			sc_rbuf[SABTTY_RBUF_SIZE];
	uint8_t			*sc_rend, *sc_rput, *sc_rget;
	uint8_t			sc_polling, sc_pollrfc;
};

struct sabtty_softc *sabtty_cons_input;
struct sabtty_softc *sabtty_cons_output;

#define	SAB_READ(sc,r)		\
    bus_space_read_1((sc)->sc_bt, (sc)->sc_bh, (r))
#define	SAB_WRITE(sc,r,v)	\
    bus_space_write_1((sc)->sc_bt, (sc)->sc_bh, (r), (v))
#define	SAB_WRITE_BLOCK(sc,r,p,c)	\
    bus_space_write_region_1((sc)->sc_bt, (sc)->sc_bh, (r), (p), (c))

int sab_match(device_t, cfdata_t, void *);
void sab_attach(device_t, device_t, void *);
int sab_print(void *, const char *);
int sab_intr(void *);

void sab_softintr(void *);
void sab_cnputc(dev_t, int);
int sab_cngetc(dev_t);
void sab_cnpollc(dev_t, int);

int sabtty_match(device_t, cfdata_t, void *);
void sabtty_attach(device_t, device_t, void *);
void sabtty_start(struct tty *);
int sabtty_param(struct tty *, struct termios *);
int sabtty_intr(struct sabtty_softc *, int *);
void sabtty_softintr(struct sabtty_softc *);
int sabtty_mdmctrl(struct sabtty_softc *, int, int);
void sabtty_cec_wait(struct sabtty_softc *);
void sabtty_tec_wait(struct sabtty_softc *);
void sabtty_reset(struct sabtty_softc *);
void sabtty_flush(struct sabtty_softc *);
int sabtty_speed(int);
void sabtty_console_flags(struct sabtty_softc *);
void sabtty_cnpollc(struct sabtty_softc *, int);
void sabtty_shutdown(void *);
int sabttyparam(struct sabtty_softc *, struct tty *, struct termios *);

#ifdef KGDB
int sab_kgdb_check(struct sabtty_softc *);
void sab_kgdb_init(struct sabtty_softc *);
#endif

void sabtty_cnputc(struct sabtty_softc *, int);
int sabtty_cngetc(struct sabtty_softc *);

CFATTACH_DECL_NEW(sab, sizeof(struct sab_softc),
    sab_match, sab_attach, NULL, NULL);

extern struct cfdriver sab_cd;

CFATTACH_DECL_NEW(sabtty, sizeof(struct sabtty_softc),
    sabtty_match, sabtty_attach, NULL, NULL);

extern struct cfdriver sabtty_cd;

dev_type_open(sabopen);
dev_type_close(sabclose);
dev_type_read(sabread);
dev_type_write(sabwrite);
dev_type_ioctl(sabioctl);
dev_type_stop(sabstop);
dev_type_tty(sabtty);
dev_type_poll(sabpoll);

static struct cnm_state sabtty_cnm_state;

const struct cdevsw sabtty_cdevsw = {
	.d_open = sabopen,
	.d_close = sabclose,
	.d_read = sabread,
	.d_write = sabwrite,
	.d_ioctl = sabioctl,
	.d_stop = sabstop,
	.d_tty = sabtty,
	.d_poll = sabpoll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

struct sabtty_rate {
	int baud;
	int n, m;
};

struct sabtty_rate sabtty_baudtable[] = {
	{      50,	35,     10 },
	{      75,	47,	9 },
	{     110,	32,	9 },
	{     134,	53,	8 },
	{     150,	47,	8 },
	{     200,	35,	8 },
	{     300,	47,	7 },
	{     600,	47,	6 },
	{    1200,	47,	5 },
	{    1800,	31,	5 },
	{    2400,	47,	4 },
	{    4800,	47,	3 },
	{    9600,	47,	2 },
	{   19200,	47,	1 },
	{   38400,	23,	1 },
	{   57600,	15,	1 },
	{  115200,	 7,	1 },
	{  230400,	 3,	1 },
	{  460800,	 1,	1 },
	{   76800,	11,	1 },
	{  153600,	 5,	1 },
	{  307200,	 3,	1 },
	{  614400,	 3,	0 },
	{  921600,	 0,	1 },
};

int
sab_match(device_t parent, cfdata_t match, void *aux)
{
	struct ebus_attach_args *ea = aux;
	char *compat;

	if (strcmp(ea->ea_name, "se") == 0 ||
	    strcmp(ea->ea_name, "FJSV,se") == 0)
		return (1);

	compat = prom_getpropstring(ea->ea_node, "compatible");
	if (compat != NULL && !strcmp(compat, "sab82532"))
		return (1);

	return (0);
}

void
sab_attach(device_t parent, device_t self, void *aux)
{
	struct sab_softc *sc = device_private(self);
	struct ebus_attach_args *ea = aux;
	uint8_t r;
	u_int i;
	int locs[SABCF_NLOCS];

	sc->sc_dev = self;
	sc->sc_bt = ea->ea_bustag;
	sc->sc_node = ea->ea_node;

	/* Use prom mapping, if available. */
	if (ea->ea_nvaddr)
		sparc_promaddr_to_handle(sc->sc_bt, ea->ea_vaddr[0],
		    &sc->sc_bh);
	else if (bus_space_map(sc->sc_bt, EBUS_ADDR_FROM_REG(&ea->ea_reg[0]),
				 ea->ea_reg[0].size, 0, &sc->sc_bh) != 0) {
		printf(": can't map register space\n");
		return;
	}

	sc->sc_ih = bus_intr_establish(ea->ea_bustag, ea->ea_intr[0],
	    IPL_TTY, sab_intr, sc);
	if (sc->sc_ih == NULL) {
		printf(": can't map interrupt\n");
		return;
	}

	sc->sc_softintr = softint_establish(SOFTINT_SERIAL, sab_softintr, sc);
	if (sc->sc_softintr == NULL) {
		printf(": can't get soft intr\n");
		return;
	}

	aprint_normal(": rev ");
	r = SAB_READ(sc, SAB_VSTR) & SAB_VSTR_VMASK;
	switch (r) {
	case SAB_VSTR_V_1:
		aprint_normal("1");
		break;
	case SAB_VSTR_V_2:
		aprint_normal("2");
		break;
	case SAB_VSTR_V_32:
		aprint_normal("3.2");
		break;
	default:
		aprint_normal("unknown(0x%x)", r);
		break;
	}
	aprint_normal("\n");
	aprint_naive(": Serial controller\n");

	/* Let current output drain */
	DELAY(100000);

	/* Set all pins, except DTR pins to be inputs */
	SAB_WRITE(sc, SAB_PCR, ~(SAB_PVR_DTR_A | SAB_PVR_DTR_B));
	/* Disable port interrupts */
	SAB_WRITE(sc, SAB_PIM, 0xff);
	SAB_WRITE(sc, SAB_PVR, SAB_PVR_DTR_A | SAB_PVR_DTR_B | SAB_PVR_MAGIC);
	SAB_WRITE(sc, SAB_IPC, SAB_IPC_ICPL);

	for (i = 0; i < SAB_NCHAN; i++) {
		struct sabtty_attach_args stax;

		stax.sbt_portno = i;

		locs[SABCF_CHANNEL] = i;

		sc->sc_child[i] = device_private(config_found_sm_loc(self,
		     "sab", locs, &stax, sab_print, config_stdsubmatch));
		if (sc->sc_child[i] != NULL)
			sc->sc_nchild++;
	}
}

int
sab_print(void *args, const char *name)
{
	struct sabtty_attach_args *sa = args;

	if (name)
		aprint_normal("sabtty at %s", name);
	aprint_normal(" port %u", sa->sbt_portno);
	return (UNCONF);
}

int
sab_intr(void *vsc)
{
	struct sab_softc *sc = vsc;
	int r = 0, needsoft = 0;
	uint8_t gis;

	gis = SAB_READ(sc, SAB_GIS);

	/* channel A */
	if ((gis & (SAB_GIS_ISA1 | SAB_GIS_ISA0)) && sc->sc_child[0] &&
	    sc->sc_child[0]->sc_tty)
		r |= sabtty_intr(sc->sc_child[0], &needsoft);

	/* channel B */
	if ((gis & (SAB_GIS_ISB1 | SAB_GIS_ISB0)) && sc->sc_child[1] &&
	    sc->sc_child[1]->sc_tty)
		r |= sabtty_intr(sc->sc_child[1], &needsoft);

	if (needsoft)
		softint_schedule(sc->sc_softintr);

	return (r);
}

void
sab_softintr(void *vsc)
{
	struct sab_softc *sc = vsc;

	if (sc->sc_child[0] && sc->sc_child[0]->sc_tty)
		sabtty_softintr(sc->sc_child[0]);
	if (sc->sc_child[1] && sc->sc_child[1]->sc_tty)
		sabtty_softintr(sc->sc_child[1]);
}

int
sabtty_match(device_t parent, cfdata_t match, void *aux)
{

	return (1);
}

void
sabtty_attach(device_t parent, device_t self, void *aux)
{
	struct sabtty_softc *sc = device_private(self);
	struct sabtty_attach_args *sa = aux;
	int r;
	int maj;
	int is_kgdb = 0;

	sc->sc_dev = self;
#ifdef KGDB
	is_kgdb = sab_kgdb_check(sc);
#endif

	if (!is_kgdb) {
		sc->sc_tty = tty_alloc();
		if (sc->sc_tty == NULL) {
			aprint_normal(": failed to allocate tty\n");
			return;
		}
		tty_attach(sc->sc_tty);
		sc->sc_tty->t_oproc = sabtty_start;
		sc->sc_tty->t_param = sabtty_param;
	}

	sc->sc_parent = device_private(parent);
	sc->sc_bt = sc->sc_parent->sc_bt;
	sc->sc_portno = sa->sbt_portno;
	sc->sc_rend = sc->sc_rbuf + SABTTY_RBUF_SIZE;

	switch (sa->sbt_portno) {
	case 0:	/* port A */
		sc->sc_pvr_dtr = SAB_PVR_DTR_A;
		sc->sc_pvr_dsr = SAB_PVR_DSR_A;
		r = bus_space_subregion(sc->sc_bt, sc->sc_parent->sc_bh,
		    SAB_CHAN_A, SAB_CHANLEN, &sc->sc_bh);
		break;
	case 1:	/* port B */
		sc->sc_pvr_dtr = SAB_PVR_DTR_B;
		sc->sc_pvr_dsr = SAB_PVR_DSR_B;
		r = bus_space_subregion(sc->sc_bt, sc->sc_parent->sc_bh,
		    SAB_CHAN_B, SAB_CHANLEN, &sc->sc_bh);
		break;
	default:
		aprint_normal(": invalid channel: %u\n", sa->sbt_portno);
		return;
	}
	if (r != 0) {
		aprint_normal(": failed to allocate register subregion\n");
		return;
	}

	sabtty_console_flags(sc);

	if (sc->sc_flags & (SABTTYF_CONS_IN | SABTTYF_CONS_OUT)) {
		struct termios t;
		const char *acc;

		/* Let residual prom output drain */
		DELAY(100000);

		switch (sc->sc_flags & (SABTTYF_CONS_IN | SABTTYF_CONS_OUT)) {
		case SABTTYF_CONS_IN:
			acc = "input";
			break;
		case SABTTYF_CONS_OUT:
			acc = "output";
			break;
		case SABTTYF_CONS_IN|SABTTYF_CONS_OUT:
		default:
			acc = "i/o";
			break;
		}

		t.c_ispeed= 0;
		if (sc->sc_flags & SABTTYF_IS_RSC)
			t.c_ospeed = 115200;
		else
			t.c_ospeed = 9600;
		t.c_cflag = CREAD | CS8 | HUPCL;
		sc->sc_tty->t_ospeed = 0;
		sabttyparam(sc, sc->sc_tty, &t);

		if (sc->sc_flags & SABTTYF_CONS_IN) {
			sabtty_cons_input = sc;
			cn_tab->cn_pollc = sab_cnpollc;
			cn_tab->cn_getc = sab_cngetc;
			maj = cdevsw_lookup_major(&sabtty_cdevsw);
			cn_tab->cn_dev = makedev(maj, device_unit(self));
			shutdownhook_establish(sabtty_shutdown, sc);
			cn_init_magic(&sabtty_cnm_state);
			cn_set_magic("\047\001"); /* default magic is BREAK */
		}

		if (sc->sc_flags & SABTTYF_CONS_OUT) {
			sabtty_tec_wait(sc);
			sabtty_cons_output = sc;
			cn_tab->cn_putc = sab_cnputc;
			maj = cdevsw_lookup_major(&sabtty_cdevsw);
			cn_tab->cn_dev = makedev(maj, device_unit(self));
		}
		aprint_normal(": console %s", acc);
	} else {
		/* Not a console... */
		sabtty_reset(sc);

#ifdef KGDB
		if (is_kgdb) {
			sab_kgdb_init(sc);
			aprint_normal(": kgdb");
		}
#endif
	}

	aprint_normal("\n");
	aprint_naive(": Serial port\n");
}

int
sabtty_intr(struct sabtty_softc *sc, int *needsoftp)
{
	uint8_t isr0, isr1;
	int i, len = 0, needsoft = 0, r = 0, clearfifo = 0;

	isr0 = SAB_READ(sc, SAB_ISR0);
	isr1 = SAB_READ(sc, SAB_ISR1);

	if (isr0 || isr1)
		r = 1;

	if (isr0 & SAB_ISR0_RPF) {
		len = 32;
		clearfifo = 1;
	}
	if (isr0 & SAB_ISR0_TCD) {
		len = (32 - 1) & SAB_READ(sc, SAB_RBCL);
		clearfifo = 1;
	}
	if (isr0 & SAB_ISR0_TIME) {
		sabtty_cec_wait(sc);
		SAB_WRITE(sc, SAB_CMDR, SAB_CMDR_RFRD);
	}
	if (isr0 & SAB_ISR0_RFO) {
		sc->sc_flags |= SABTTYF_RINGOVERFLOW;
		clearfifo = 1;
	}
	if (len != 0) {
		uint8_t *ptr, b;

		ptr = sc->sc_rput;
		for (i = 0; i < len; i++) {
			b = SAB_READ(sc, SAB_RFIFO);
			if (i % 2 == 0) /* skip status byte */
				cn_check_magic(sc->sc_tty->t_dev,
					       b, sabtty_cnm_state);
			*ptr++ = b;
			if (ptr == sc->sc_rend)
				ptr = sc->sc_rbuf;
			if (ptr == sc->sc_rget) {
				if (ptr == sc->sc_rbuf)
					ptr = sc->sc_rend;
				ptr--;
				sc->sc_flags |= SABTTYF_RINGOVERFLOW;
			}
		}
		sc->sc_rput = ptr;
		needsoft = 1;
	}

	if (clearfifo) {
		sabtty_cec_wait(sc);
		SAB_WRITE(sc, SAB_CMDR, SAB_CMDR_RMC);
	}

	if (isr0 & SAB_ISR0_CDSC) {
		sc->sc_flags |= SABTTYF_CDCHG;
		needsoft = 1;
	}

	if (isr1 & SAB_ISR1_BRKT)
		cn_check_magic(sc->sc_tty->t_dev,
			       CNC_BREAK, sabtty_cnm_state);

	if (isr1 & (SAB_ISR1_XPR | SAB_ISR1_ALLS)) {
		if ((SAB_READ(sc, SAB_STAR) & SAB_STAR_XFW) &&
		    (sc->sc_flags & SABTTYF_STOP) == 0) {
			if (sc->sc_txc < 32)
				len = sc->sc_txc;
			else
				len = 32;

			if (len > 0) {
				SAB_WRITE_BLOCK(sc, SAB_XFIFO, sc->sc_txp, len);
				sc->sc_txp += len;
				sc->sc_txc -= len;

				sabtty_cec_wait(sc);
				SAB_WRITE(sc, SAB_CMDR, SAB_CMDR_XF);

				/*
				 * Prevent the false end of xmit from
				 * confusing things below.
				 */
				isr1 &= ~SAB_ISR1_ALLS;
			}
		}

		if ((sc->sc_txc == 0) || (sc->sc_flags & SABTTYF_STOP)) {
			if ((sc->sc_imr1 & SAB_IMR1_XPR) == 0) {
				sc->sc_imr1 |= SAB_IMR1_XPR;
				sc->sc_imr1 &= ~SAB_IMR1_ALLS;
				SAB_WRITE(sc, SAB_IMR1, sc->sc_imr1);
			}
		}
	}

	if ((isr1 & SAB_ISR1_ALLS) && ((sc->sc_txc == 0) ||
	    (sc->sc_flags & SABTTYF_STOP))) {
		if (sc->sc_flags & SABTTYF_TXDRAIN)
			wakeup(sc);
		sc->sc_flags &= ~SABTTYF_STOP;
		sc->sc_flags |= SABTTYF_DONE;
		sc->sc_imr1 |= SAB_IMR1_ALLS;
		SAB_WRITE(sc, SAB_IMR1, sc->sc_imr1);
		needsoft = 1;
	}

	if (needsoft)
		*needsoftp = needsoft;
	return (r);
}

void
sabtty_softintr(struct sabtty_softc *sc)
{
	struct tty *tp = sc->sc_tty;
	int s, flags;
	uint8_t r;

	if (tp == NULL)
		return;

	if ((tp->t_state & TS_ISOPEN) == 0)
		return;

	while (sc->sc_rget != sc->sc_rput) {
		int data;
		uint8_t stat;

		data = sc->sc_rget[0];
		stat = sc->sc_rget[1];
		sc->sc_rget += 2;
		if (stat & SAB_RSTAT_PE)
			data |= TTY_PE;
		if (stat & SAB_RSTAT_FE)
			data |= TTY_FE;
		if (sc->sc_rget == sc->sc_rend)
			sc->sc_rget = sc->sc_rbuf;

		(*tp->t_linesw->l_rint)(data, tp);
	}

	s = splhigh();
	flags = sc->sc_flags;
	sc->sc_flags &= ~(SABTTYF_DONE|SABTTYF_CDCHG|SABTTYF_RINGOVERFLOW);
	splx(s);

	if (flags & SABTTYF_CDCHG) {
		s = spltty();
		r = SAB_READ(sc, SAB_VSTR) & SAB_VSTR_CD;
		splx(s);

		(*tp->t_linesw->l_modem)(tp, r);
	}

	if (flags & SABTTYF_RINGOVERFLOW)
		log(LOG_WARNING, "%s: ring overflow\n",
		    device_xname(sc->sc_dev));

	if (flags & SABTTYF_DONE) {
		ndflush(&tp->t_outq, sc->sc_txp - tp->t_outq.c_cf);
		tp->t_state &= ~TS_BUSY;
		(*tp->t_linesw->l_start)(tp);
	}
}

int
sabopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct sabtty_softc *sc;
	struct tty *tp;
	int s, s1;

	sc = device_lookup_private(&sabtty_cd, SABUNIT(dev));
	if (sc == NULL)
		return (ENXIO);

	tp = sc->sc_tty;
	tp->t_dev = dev;

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
		return (EBUSY);

	mutex_spin_enter(&tty_lock);
	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		if (sc->sc_openflags & TIOCFLAG_CLOCAL)
			tp->t_cflag |= CLOCAL;
		if (sc->sc_openflags & TIOCFLAG_CRTSCTS)
			tp->t_cflag |= CRTSCTS;
		if (sc->sc_openflags & TIOCFLAG_MDMBUF)
			tp->t_cflag |= MDMBUF;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;

		sc->sc_rput = sc->sc_rget = sc->sc_rbuf;

		ttsetwater(tp);

		s1 = splhigh();
		sabtty_reset(sc);
		sabtty_param(tp, &tp->t_termios);
		sc->sc_imr0 = SAB_IMR0_PERR | SAB_IMR0_FERR | SAB_IMR0_PLLA;
		SAB_WRITE(sc, SAB_IMR0, sc->sc_imr0);
		sc->sc_imr1 = SAB_IMR1_BRK | SAB_IMR1_ALLS | SAB_IMR1_XDU |
		    SAB_IMR1_TIN | SAB_IMR1_CSC | SAB_IMR1_XMR | SAB_IMR1_XPR;
		SAB_WRITE(sc, SAB_IMR1, sc->sc_imr1);
		SAB_WRITE(sc, SAB_CCR0, SAB_READ(sc, SAB_CCR0) | SAB_CCR0_PU);
		sabtty_cec_wait(sc);
		SAB_WRITE(sc, SAB_CMDR, SAB_CMDR_XRES);
		sabtty_cec_wait(sc);
		SAB_WRITE(sc, SAB_CMDR, SAB_CMDR_RRES);
		sabtty_cec_wait(sc);
		splx(s1);

		sabtty_flush(sc);

		if ((sc->sc_openflags & TIOCFLAG_SOFTCAR) ||
		    (SAB_READ(sc, SAB_VSTR) & SAB_VSTR_CD))
			tp->t_state |= TS_CARR_ON;
		else
			tp->t_state &= ~TS_CARR_ON;
	}

	if ((flags & O_NONBLOCK) == 0) {
		while ((tp->t_cflag & CLOCAL) == 0 &&
		    (tp->t_state & TS_CARR_ON) == 0) {
			int error;

			error = ttysleep(tp, &tp->t_rawcv, true, 0);
			if (error != 0) {
				mutex_spin_exit(&tty_lock);
				return (error);
			}
		}
	}

	mutex_spin_exit(&tty_lock);

	s = (*tp->t_linesw->l_open)(dev, tp);
	if (s != 0) {
		mutex_spin_enter(&tty_lock);
		if (tp->t_state & TS_ISOPEN) {
			mutex_spin_exit(&tty_lock);
			return (s);
		}
		if (tp->t_cflag & HUPCL) {
			sabtty_mdmctrl(sc, 0, DMSET);
			cv_wait(&lbolt, &tty_lock);
		}

		if ((sc->sc_flags & (SABTTYF_CONS_IN | SABTTYF_CONS_OUT)) == 0) {
			/* Flush and power down if we're not the console */
			sabtty_flush(sc);
			sabtty_reset(sc);
		}
		mutex_spin_exit(&tty_lock);
	}
	return (s);
}

int
sabclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct sabtty_softc *sc = device_lookup_private(&sabtty_cd, SABUNIT(dev));
	struct sab_softc *bc = sc->sc_parent;
	struct tty *tp = sc->sc_tty;
	int s;

	(*tp->t_linesw->l_close)(tp, flags);

	s = spltty();

	if ((tp->t_state & TS_ISOPEN) == 0) {
		/* Wait for output drain */
		sc->sc_imr1 &= ~SAB_IMR1_ALLS;
		SAB_WRITE(sc, SAB_IMR1, sc->sc_imr1);
		sc->sc_flags |= SABTTYF_TXDRAIN;
		(void)tsleep(sc, TTIPRI, ttclos, 5 * hz);
		sc->sc_imr1 |= SAB_IMR1_ALLS;
		SAB_WRITE(sc, SAB_IMR1, sc->sc_imr1);
		sc->sc_flags &= ~SABTTYF_TXDRAIN;

		if (tp->t_cflag & HUPCL) {
			sabtty_mdmctrl(sc, 0, DMSET);
			(void)tsleep(bc, TTIPRI, ttclos, hz);
		}

		if ((sc->sc_flags & (SABTTYF_CONS_IN | SABTTYF_CONS_OUT)) == 0) {
			/* Flush and power down if we're not the console */
			sabtty_flush(sc);
			sabtty_reset(sc);
		}
	}

	ttyclose(tp);
	splx(s);

	return (0);
}

int
sabread(dev_t dev, struct uio *uio, int flags)
{
	struct sabtty_softc *sc = device_lookup_private(&sabtty_cd, SABUNIT(dev));
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_read)(tp, uio, flags));
}

int
sabwrite(dev_t dev, struct uio *uio, int flags)
{
	struct sabtty_softc *sc = device_lookup_private(&sabtty_cd, SABUNIT(dev));
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_write)(tp, uio, flags));
}

int
sabioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	struct sabtty_softc *sc = device_lookup_private(&sabtty_cd, SABUNIT(dev));
	struct tty *tp = sc->sc_tty;
	int error;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flags, l);
	if (error >= 0)
		return (error);

	error = ttioctl(tp, cmd, data, flags, l);
	if (error >= 0)
		return (error);

	error = 0;

	switch (cmd) {
	case TIOCSBRK:
		SAB_WRITE(sc, SAB_DAFO,
		    SAB_READ(sc, SAB_DAFO) | SAB_DAFO_XBRK);
		break;
	case TIOCCBRK:
		SAB_WRITE(sc, SAB_DAFO,
		    SAB_READ(sc, SAB_DAFO) & ~SAB_DAFO_XBRK);
		break;
	case TIOCSDTR:
		sabtty_mdmctrl(sc, TIOCM_DTR, DMBIS);
		break;
	case TIOCCDTR:
		sabtty_mdmctrl(sc, TIOCM_DTR, DMBIC);
		break;
	case TIOCMBIS:
		sabtty_mdmctrl(sc, *((int *)data), DMBIS);
		break;
	case TIOCMBIC:
		sabtty_mdmctrl(sc, *((int *)data), DMBIC);
		break;
	case TIOCMGET:
		*((int *)data) = sabtty_mdmctrl(sc, 0, DMGET);
		break;
	case TIOCMSET:
		sabtty_mdmctrl(sc, *((int *)data), DMSET);
		break;
	case TIOCGFLAGS:
		*((int *)data) = sc->sc_openflags;
		break;
	case TIOCSFLAGS:
		if (kauth_authorize_device_tty(l->l_cred,
		    KAUTH_DEVICE_TTY_PRIVSET, tp))
			error = EPERM;
		else
			sc->sc_openflags = *((int *)data) &
			    (TIOCFLAG_SOFTCAR | TIOCFLAG_CLOCAL |
			     TIOCFLAG_CRTSCTS | TIOCFLAG_MDMBUF);
		break;
	default:
		error = ENOTTY;
	}

	return (error);
}

struct tty *
sabtty(dev_t dev)
{
	struct sabtty_softc *sc = device_lookup_private(&sabtty_cd, SABUNIT(dev));

	return (sc->sc_tty);
}

void
sabstop(struct tty *tp, int flag)
{
	struct sabtty_softc *sc = device_lookup_private(&sabtty_cd, SABUNIT(tp->t_dev));
	int s;

	s = spltty();
	if (tp->t_state & TS_BUSY) {
		if ((tp->t_state & TS_TTSTOP) == 0)
			tp->t_state |= TS_FLUSH;
		sc->sc_flags |= SABTTYF_STOP;
		sc->sc_imr1 &= ~SAB_IMR1_ALLS;
		SAB_WRITE(sc, SAB_IMR1, sc->sc_imr1);
	}
	splx(s);
}

int
sabpoll(dev_t dev, int events, struct lwp *l)
{
	struct sabtty_softc *sc = device_lookup_private(&sabtty_cd, SABUNIT(dev));
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

int
sabtty_mdmctrl(struct sabtty_softc *sc, int bits, int how)
{
	uint8_t r;
	int s;

	s = spltty();
	switch (how) {
	case DMGET:
		bits = 0;
		if (SAB_READ(sc, SAB_STAR) & SAB_STAR_CTS)
			bits |= TIOCM_CTS;
		if ((SAB_READ(sc, SAB_VSTR) & SAB_VSTR_CD) == 0)
			bits |= TIOCM_CD;

		r = SAB_READ(sc, SAB_PVR);
		if ((r & sc->sc_pvr_dtr) == 0)
			bits |= TIOCM_DTR;
		if ((r & sc->sc_pvr_dsr) == 0)
			bits |= TIOCM_DSR;

		r = SAB_READ(sc, SAB_MODE);
		if ((r & (SAB_MODE_RTS|SAB_MODE_FRTS)) == SAB_MODE_RTS)
			bits |= TIOCM_RTS;
		break;
	case DMSET:
		r = SAB_READ(sc, SAB_MODE);
		if (bits & TIOCM_RTS) {
			r &= ~SAB_MODE_FRTS;
			r |= SAB_MODE_RTS;
		} else
			r |= SAB_MODE_FRTS | SAB_MODE_RTS;
		SAB_WRITE(sc, SAB_MODE, r);

		r = SAB_READ(sc, SAB_PVR);
		if (bits & TIOCM_DTR)
			r &= ~sc->sc_pvr_dtr;
		else
			r |= sc->sc_pvr_dtr;
		SAB_WRITE(sc, SAB_PVR, r);
		break;
	case DMBIS:
		if (bits & TIOCM_RTS) {
			r = SAB_READ(sc, SAB_MODE);
			r &= ~SAB_MODE_FRTS;
			r |= SAB_MODE_RTS;
			SAB_WRITE(sc, SAB_MODE, r);
		}
		if (bits & TIOCM_DTR) {
			r = SAB_READ(sc, SAB_PVR);
			r &= ~sc->sc_pvr_dtr;
			SAB_WRITE(sc, SAB_PVR, r);
		}
		break;
	case DMBIC:
		if (bits & TIOCM_RTS) {
			r = SAB_READ(sc, SAB_MODE);
			r |= SAB_MODE_FRTS | SAB_MODE_RTS;
			SAB_WRITE(sc, SAB_MODE, r);
		}
		if (bits & TIOCM_DTR) {
			r = SAB_READ(sc, SAB_PVR);
			r |= sc->sc_pvr_dtr;
			SAB_WRITE(sc, SAB_PVR, r);
		}
		break;
	}
	splx(s);
	return (bits);
}

int
sabttyparam(struct sabtty_softc *sc, struct tty *tp, struct termios *t)
{
	int s, ospeed;
	tcflag_t cflag;
	uint8_t dafo, r;

	ospeed = sabtty_speed(t->c_ospeed);
	if (ospeed < 0 || (t->c_ispeed && t->c_ispeed != t->c_ospeed))
		return (EINVAL);

	s = spltty();

	/* hang up line if ospeed is zero, otherwise raise dtr */
	sabtty_mdmctrl(sc, TIOCM_DTR,
	    (t->c_ospeed == 0) ? DMBIC : DMBIS);

	dafo = SAB_READ(sc, SAB_DAFO);

	cflag = t->c_cflag;

	if (sc->sc_flags & (SABTTYF_CONS_IN | SABTTYF_CONS_OUT)) {
		cflag |= CLOCAL;
		cflag &= ~HUPCL;
	}

	if (cflag & CSTOPB)
		dafo |= SAB_DAFO_STOP;
	else
		dafo &= ~SAB_DAFO_STOP;

	dafo &= ~SAB_DAFO_CHL_CSIZE;
	switch (cflag & CSIZE) {
	case CS5:
		dafo |= SAB_DAFO_CHL_CS5;
		break;
	case CS6:
		dafo |= SAB_DAFO_CHL_CS6;
		break;
	case CS7:
		dafo |= SAB_DAFO_CHL_CS7;
		break;
	default:
		dafo |= SAB_DAFO_CHL_CS8;
		break;
	}

	dafo &= ~SAB_DAFO_PARMASK;
	if (cflag & PARENB) {
		if (cflag & PARODD)
			dafo |= SAB_DAFO_PAR_ODD;
		else
			dafo |= SAB_DAFO_PAR_EVEN;
	} else
		dafo |= SAB_DAFO_PAR_NONE;
	SAB_WRITE(sc, SAB_DAFO, dafo);

	if (!(sc->sc_flags & SABTTYF_IS_RSC) && ospeed != 0) {
		SAB_WRITE(sc, SAB_BGR, ospeed & 0xff);
		r = SAB_READ(sc, SAB_CCR2);
		r &= ~(SAB_CCR2_BR9 | SAB_CCR2_BR8);
		r |= (ospeed >> 2) & (SAB_CCR2_BR9 | SAB_CCR2_BR8);
		SAB_WRITE(sc, SAB_CCR2, r);
	}

	r = SAB_READ(sc, SAB_MODE);
	r |= SAB_MODE_RAC;
	if (cflag & CRTSCTS) {
		r &= ~(SAB_MODE_RTS | SAB_MODE_FCTS);
		r |= SAB_MODE_FRTS;
		sc->sc_imr1 &= ~SAB_IMR1_CSC;
	} else {
		r |= SAB_MODE_RTS | SAB_MODE_FCTS;
		r &= ~SAB_MODE_FRTS;
		sc->sc_imr1 |= SAB_IMR1_CSC;
	}
	SAB_WRITE(sc, SAB_MODE, r);
	SAB_WRITE(sc, SAB_IMR1, sc->sc_imr1);

	tp->t_cflag = cflag;

	splx(s);
	return (0);
}

int
sabtty_param(struct tty *tp, struct termios *t)
{
	struct sabtty_softc *sc = device_lookup_private(&sabtty_cd, SABUNIT(tp->t_dev));

	return (sabttyparam(sc, tp, t));
}

void
sabtty_start(struct tty *tp)
{
	struct sabtty_softc *sc = device_lookup_private(&sabtty_cd, SABUNIT(tp->t_dev));
	int s;

	s = spltty();
	if ((tp->t_state & (TS_TTSTOP | TS_TIMEOUT | TS_BUSY)) == 0) {
		if (ttypull(tp)) {
			sc->sc_txc = ndqb(&tp->t_outq, 0);
			sc->sc_txp = tp->t_outq.c_cf;
			tp->t_state |= TS_BUSY;
			sc->sc_imr1 &= ~(SAB_ISR1_XPR | SAB_ISR1_ALLS);
			SAB_WRITE(sc, SAB_IMR1, sc->sc_imr1);
		}
	}
	splx(s);
}

void
sabtty_cec_wait(struct sabtty_softc *sc)
{
	int i = 50000;

	for (;;) {
		if ((SAB_READ(sc, SAB_STAR) & SAB_STAR_CEC) == 0)
			return;
		if (--i == 0)
			break;
		DELAY(1);
	}
}

void
sabtty_tec_wait(struct sabtty_softc *sc)
{
	int i = 200000;

	for (;;) {
		if ((SAB_READ(sc, SAB_STAR) & SAB_STAR_TEC) == 0)
			return;
		if (--i == 0)
			break;
		DELAY(1);
	}
}

void
sabtty_reset(struct sabtty_softc *sc)
{
	/* power down */
	SAB_WRITE(sc, SAB_CCR0, 0);

	/* set basic configuration */
	SAB_WRITE(sc, SAB_CCR0,
	    SAB_CCR0_MCE | SAB_CCR0_SC_NRZ | SAB_CCR0_SM_ASYNC);
	SAB_WRITE(sc, SAB_CCR1, SAB_CCR1_ODS | SAB_CCR1_BCR | SAB_CCR1_CM_7);
	SAB_WRITE(sc, SAB_CCR2, SAB_CCR2_BDF | SAB_CCR2_SSEL | SAB_CCR2_TOE);
	SAB_WRITE(sc, SAB_CCR3, 0);
	SAB_WRITE(sc, SAB_CCR4, SAB_CCR4_MCK4 | SAB_CCR4_EBRG | SAB_CCR4_ICD);
	SAB_WRITE(sc, SAB_MODE, SAB_MODE_RTS | SAB_MODE_FCTS | SAB_MODE_RAC);
	SAB_WRITE(sc, SAB_RFC,
	    SAB_RFC_DPS | SAB_RFC_RFDF | SAB_RFC_RFTH_32CHAR);

	/* clear interrupts */
	sc->sc_imr0 = sc->sc_imr1 = 0xff;
	SAB_WRITE(sc, SAB_IMR0, sc->sc_imr0);
	SAB_WRITE(sc, SAB_IMR1, sc->sc_imr1);
	(void)SAB_READ(sc, SAB_ISR0);
	(void)SAB_READ(sc, SAB_ISR1);
}

void
sabtty_flush(struct sabtty_softc *sc)
{
	/* clear rx fifo */
	sabtty_cec_wait(sc);
	SAB_WRITE(sc, SAB_CMDR, SAB_CMDR_RRES);

	/* clear tx fifo */
	sabtty_cec_wait(sc);
	SAB_WRITE(sc, SAB_CMDR, SAB_CMDR_XRES);
}

int
sabtty_speed(int rate)
{
	int i, len, r;

	if (rate == 0)
		return (0);
	len = sizeof(sabtty_baudtable)/sizeof(sabtty_baudtable[0]);
	for (i = 0; i < len; i++) {
		if (rate == sabtty_baudtable[i].baud) {
			r = sabtty_baudtable[i].n |
			    (sabtty_baudtable[i].m << 6);
			return (r);
		}
	}
	return (-1);
}

void
sabtty_cnputc(struct sabtty_softc *sc, int c)
{
	sabtty_tec_wait(sc);
	SAB_WRITE(sc, SAB_TIC, c);
	sabtty_tec_wait(sc);
}

int
sabtty_cngetc(struct sabtty_softc *sc)
{
	uint8_t r, len;

again:
	do {
		r = SAB_READ(sc, SAB_STAR);
	} while ((r & SAB_STAR_RFNE) == 0);

	/*
	 * Ok, at least one byte in RFIFO, ask for permission to access RFIFO
	 * (I hate this chip... hate hate hate).
	 */
	sabtty_cec_wait(sc);
	SAB_WRITE(sc, SAB_CMDR, SAB_CMDR_RFRD);

	/* Wait for RFIFO to come ready */
	do {
		r = SAB_READ(sc, SAB_ISR0);
	} while ((r & SAB_ISR0_TCD) == 0);

	len = SAB_READ(sc, SAB_RBCL) & (32 - 1);
	if (len == 0)
		goto again;	/* Shouldn't happen... */

	r = SAB_READ(sc, SAB_RFIFO);

	/*
	 * Blow away everything left in the FIFO...
	 */
	sabtty_cec_wait(sc);
	SAB_WRITE(sc, SAB_CMDR, SAB_CMDR_RMC);
	return (r);
}

void
sabtty_cnpollc(struct sabtty_softc *sc, int on)
{
	uint8_t r;

	if (on) {
		if (sc->sc_polling)
			return;
		SAB_WRITE(sc, SAB_IPC, SAB_READ(sc, SAB_IPC) | SAB_IPC_VIS);
		r = sc->sc_pollrfc = SAB_READ(sc, SAB_RFC);
		r &= ~(SAB_RFC_RFDF);
		SAB_WRITE(sc, SAB_RFC, r);
		sabtty_cec_wait(sc);
		SAB_WRITE(sc, SAB_CMDR, SAB_CMDR_RRES);
		sc->sc_polling = 1;
	} else {
		if (!sc->sc_polling)
			return;
		SAB_WRITE(sc, SAB_IPC, SAB_READ(sc, SAB_IPC) & ~SAB_IPC_VIS);
		SAB_WRITE(sc, SAB_RFC, sc->sc_pollrfc);
		sabtty_cec_wait(sc);
		SAB_WRITE(sc, SAB_CMDR, SAB_CMDR_RRES);
		sc->sc_polling = 0;
	}
}

void
sab_cnputc(dev_t dev, int c)
{
	struct sabtty_softc *sc = sabtty_cons_output;

	if (sc == NULL)
		return;
	sabtty_cnputc(sc, c);
}

void
sab_cnpollc(dev_t dev, int on)
{
	struct sabtty_softc *sc = sabtty_cons_input;

	sabtty_cnpollc(sc, on);
}

int
sab_cngetc(dev_t dev)
{
	struct sabtty_softc *sc = sabtty_cons_input;

	if (sc == NULL)
		return (-1);
	return (sabtty_cngetc(sc));
}

void
sabtty_console_flags(struct sabtty_softc *sc)
{
	int node, channel, cookie;
	char buf[255];

	node = sc->sc_parent->sc_node;
	channel = sc->sc_portno;

	/* Default to channel 0 if there are no explicit prom args */
	cookie = 0;

	if (node == prom_instance_to_package(prom_stdin())) {
		if (prom_getoption("input-device", buf, sizeof buf) == 0 &&
			strcmp("ttyb", buf) == 0)
				cookie = 1;

		if (channel == cookie)
			sc->sc_flags |= SABTTYF_CONS_IN;
	}

	/* Default to same channel if there are no explicit prom args */

	if (node == prom_instance_to_package(prom_stdout())) {
		if (prom_getoption("output-device", buf, sizeof buf) == 0 &&
			strcmp("ttyb", buf) == 0)
				cookie = 1;

		if (channel == cookie)
			sc->sc_flags |= SABTTYF_CONS_OUT;
	}
	/* Are we connected to an E250 RSC? */
	if (channel == prom_getpropint(node, "ssp-console", -1) ||
	    channel == prom_getpropint(node, "ssp-control", -1))
		sc->sc_flags |= SABTTYF_IS_RSC;
}

void
sabtty_shutdown(void *vsc)
{
	struct sabtty_softc *sc = vsc;

	/* Have to put the chip back into single char mode */
	sc->sc_flags |= SABTTYF_DONTDDB;
	SAB_WRITE(sc, SAB_RFC, SAB_READ(sc, SAB_RFC) & ~SAB_RFC_RFDF);
	sabtty_cec_wait(sc);
	SAB_WRITE(sc, SAB_CMDR, SAB_CMDR_RRES);
	sabtty_cec_wait(sc);
}

#ifdef KGDB
static int
sab_kgdb_getc(void *arg)
{
	struct sabtty_softc *sc = arg;

	return sabtty_cngetc(sc);
}

static void
sab_kgdb_putc(void *arg, int c)
{
	struct sabtty_softc *sc = arg;

	return sabtty_cnputc(sc, c);
}

#ifndef KGDB_DEVRATE
#define KGDB_DEVRATE    9600
#endif

int
sab_kgdb_check(struct sabtty_softc *sc)
{
	return strcmp(device_xname(sc->sc_dev), KGDB_DEVNAME) == 0;
}

void
sab_kgdb_init(struct sabtty_softc *sc)
{
	int sp;
	uint8_t dafo, r;
	extern int kgdb_break_at_attach;

	kgdb_attach(sab_kgdb_getc, sab_kgdb_putc, sc);
	kgdb_dev = 123;	/* not used, but checked against NODEV */

	/*
	 * Configure the port to speed/8n1
	 */
	dafo = SAB_READ(sc, SAB_DAFO);
	dafo &= ~(SAB_DAFO_STOP|SAB_DAFO_CHL_CSIZE|SAB_DAFO_PARMASK);
	dafo |= SAB_DAFO_CHL_CS8|SAB_DAFO_PAR_NONE;
	SAB_WRITE(sc, SAB_DAFO, dafo);
	sp = sabtty_speed(KGDB_DEVRATE);
	SAB_WRITE(sc, SAB_BGR, sp & 0xff);
	r = SAB_READ(sc, SAB_CCR2);
	r &= ~(SAB_CCR2_BR9 | SAB_CCR2_BR8);
	r |= (sp >> 2) & (SAB_CCR2_BR9 | SAB_CCR2_BR8);
	SAB_WRITE(sc, SAB_CCR2, r);

	/*
	 * If we are booting with -d, break into kgdb now
	 */
	if (kgdb_break_at_attach)
		kgdb_connect(1);
}
#endif

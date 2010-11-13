/* $Id: imxuart.c,v 1.4 2010/11/13 05:00:31 bsh Exp $ */
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>

#include <arm/armreg.h>

#include <sys/tty.h>
#include <sys/termios.h>
#include <dev/cons.h>

#include <machine/bus.h>
#include <arm/imx/imx31var.h>
#include <arm/imx/imxuartreg.h>
#include <arm/imx/imxuartvar.h>
#include <evbarm/imx31/imx31lk_reg.h>

#define IMXUART_UNIT_MASK	0x7ffff
#define IMXUART_DIALOUT_MASK	0x80000

#define IMXUART_UNIT(dev)	(minor(dev) & IMXUART_UNIT_MASK)
#define IMXUART_DIALOUT(dev)	(minor(dev) & IMXUART_DIALOUT_MASK)



#define __TRACE	imxuart_puts(&imxuart_softc, __func__ )

extern struct bus_space imx_bs_tag;

imxuart_softc_t imxuart_softc = { { 0, }, };

uint32_t imxuart_snapshot_data[32];
const char *imxuart_test_string = 
	"0123456789012345678901234567890123456789\r\n";


cons_decl(imxuart);
static struct consdev imxuartcntab = cons_init(imxuart);

#ifdef NOTYET
dev_type_open(imxuartopen);
dev_type_close(imxuartclose);
dev_type_read(imxuartread);
dev_type_write(imxuartwrite);
dev_type_ioctl(imxuartioctl);
dev_type_stop(imxuartstop); 
dev_type_tty(imxuarttty);
dev_type_poll(imxuartpoll);
const struct cdevsw imxuart_cdevsw = {
        imxuartopen, imxuartclose, imxuartread, imxuartwrite,
	imxuartioctl, imxuartstop, imxuarttty, imxuartpoll,
	nommap, ttykqfilter, D_TTY
}; 
#else
const struct cdevsw imxuart_cdevsw = {
        nullopen, nullclose, nullread, nullwrite,
	nullioctl, nullstop, notty, nullpoll,
	nommap, nullkqfilter, D_TTY
}; 
#endif


int  imxuart_cnattach(bus_space_tag_t, bus_addr_t, int, int, int, tcflag_t);
int  imxuart_puts(imxuart_softc_t *, const char *);
int  imxuart_putchar(imxuart_softc_t *, int);
int  imxuart_getchar(imxuart_softc_t *);

static int  imxuart_urxd_brk(void);
static void imxuart_urxd_err(imxuart_softc_t *, uint32_t);

static int imxuart_match(struct device *, struct cfdata *, void *);
static void imxuart_attach(struct device *, struct device *, void *);

static void imxuart_start(struct tty *);
static int  imxuart_param(struct tty *, struct termios *);
static void imxuart_shutdownhook(void *arg);

CFATTACH_DECL(imxuart, sizeof(struct imxuart_softc),
    imxuart_match, imxuart_attach, NULL, NULL);


/*
 * disable the specified IMX UART interrupt in softc
 * return -1 on error
 * else return previous enabled state
 */
static __inline int
imxuart_intrspec_dis(imxuart_softc_t *sc, imxuart_intrix_t ix)
{
	const imxuart_intrspec_t *spec = &imxuart_intrspec_tab[ix];
	uint32_t v;
	uint32_t b;
	const uint32_t syncbit = 1 << 31;
	uint n;

	if (ix >= IMXUART_INTRSPEC_TAB_SZ)
		return -1;

	v = (1 << ix);
	if ((sc->sc_intrspec_enb & v) == 0)
		return 0;
	sc->sc_intrspec_enb &= ~v;

	n = spec->enb_reg;
	b = spec->enb_bit;
	v = sc->sc_ucr[n];
	v &= ~b;
	v |= syncbit;
	sc->sc_ucr[n] = v;

	n = spec->flg_reg;
	b = spec->flg_bit;
	v = sc->sc_usr[n];
	v &= ~b;
	v |= syncbit;
	sc->sc_usr[n] = v;

	return 1;
}

/*
 * enable the specified IMX UART interrupt in softc
 * return -1 on error
 * else return previous enabled state
 */
static __inline int
imxuart_intrspec_enb(imxuart_softc_t *sc, imxuart_intrix_t ix)
{
	const imxuart_intrspec_t *spec = &imxuart_intrspec_tab[ix];
	uint32_t v;
	uint n;
	const uint32_t syncbit = 1 << 31;

	if (ix >= IMXUART_INTRSPEC_TAB_SZ)
		return -1;

	v = (1 << ix);
	if ((sc->sc_intrspec_enb & v) != 0)
		return 1;
	sc->sc_intrspec_enb |= v;


	n = spec->enb_reg;
	v = spec->enb_bit;
	v |= syncbit;
	sc->sc_ucr[n] |= v;

	n = spec->flg_reg;
	v = spec->flg_bit;
	v |= syncbit;
	sc->sc_usr[n] |= v;

	return 0;
}

/*
 * sync softc interrupt spec to UART Control regs
 */
static __inline void
imxuart_intrspec_sync(imxuart_softc_t *sc)
{
	int i;
	uint32_t r;
	uint32_t v;
	const uint32_t syncbit = 1 << 31;
	const uint32_t mask[4] = { 
		IMXUART_INTRS_UCR1, 
		IMXUART_INTRS_UCR2, 
		IMXUART_INTRS_UCR3, 
		IMXUART_INTRS_UCR4
	};

	for (i=0; i < 4; i++) {
		v = sc->sc_ucr[i];
		if (v & syncbit) {
			v &= ~syncbit;
			sc->sc_ucr[i] = v;
			r = bus_space_read_4(sc->sc_bt, sc->sc_bh, IMX_UCRn(i));
			r &= ~mask[i];
			r |= v;
			bus_space_write_4(sc->sc_bt, sc->sc_bh, IMX_UCRn(i), r);
		}
	}
}


int
imxuart_init(imxuart_softc_t *sc, uint bh)
{
	if (sc == 0)
		sc = &imxuart_softc;
	sc->sc_init_cnt++;
	cn_tab = &imxuartcntab;
	sc->sc_bt = &imx31_bs_tag;
	sc->sc_bh = bh;

	memset(&sc->sc_errors, 0, sizeof(sc->sc_errors));

	return 0;
}

int
imxuart_puts(imxuart_softc_t *sc, const char *s)
{
	char c;
	int err = -2;

	for(;;) {
		c = *s++;
		if (c == '\0')
			break;
		err = imxuart_putchar(sc, c);
	}
	return err;
}

int
imxuart_putchar(imxuart_softc_t *sc, int c)
{
	uint32_t r;

	for (;;) {
		r = bus_space_read_4(sc->sc_bt, sc->sc_bh, IMX_UTS);
		if ((r & IMX_UTS_TXFUL) == 0)
			break;
	}

	r = (uint32_t)c & IMX_UTXD_TX_DATA;
	bus_space_write_4(sc->sc_bt, sc->sc_bh, IMX_UTXD, r);

	return 0;
}

int
imxuart_getchar(imxuart_softc_t *sc)
{
	uint32_t r;
	int c;

	for(;;) {
		r = bus_space_read_4(sc->sc_bt, sc->sc_bh, IMX_URXD);
		if (r & IMX_URXD_ERR) {
			imxuart_urxd_err(sc, r);
			continue;
		}
		if (r & IMX_URXD_CHARDY) {
			c = (int)(r & IMX_URXD_RX_DATA);
			break;
		}
	}

	return c;
}

static int
imxuart_urxd_brk(void)
{
#ifdef DDB
	if (cn_tab == &imxuartcntab) {
		Debugger();
		return 0;
	}
#endif
	return 1;
}

static void
imxuart_urxd_err(imxuart_softc_t *sc, uint32_t r)
{
	if (r & IMX_URXD_BRK)
		if (imxuart_urxd_brk() == 0)
			return;

	sc->sc_errors.err++;
	if (r & IMX_URXD_BRK)
		sc->sc_errors.brk++;
	if (r & IMX_URXD_PRERR)
		sc->sc_errors.prerr++;
	if (r & IMX_URXD_FRMERR)
		sc->sc_errors.frmerr++;
	if (r & IMX_URXD_OVRRUN)
		sc->sc_errors.ovrrun++;
}

static int
imxuart_snapshot(imxuart_softc_t *sc, uint32_t *p)
{
	int i;
	const uint r[] = {	IMX_URXD, IMX_UTXD, IMX_UCR1, IMX_UCR2,
			IMX_UCR3, IMX_UCR4, IMX_UFCR, IMX_USR1,
			IMX_USR2, IMX_UESC, IMX_UTIM, IMX_UBIR,
			IMX_UBMR, IMX_UBRC, IMX_ONEMS, IMX_UTS };

	for (i=0; i < ((sizeof(r)/sizeof(r[0]))); i++) {
		*p++ = sc->sc_bh + r[i];
		*p++ = bus_space_read_4(sc->sc_bt, sc->sc_bh, r[i]);
	}
	return 0;
}

int
imxuart_test(void)
{
	imxuart_softc_t *sc = &imxuart_softc;
	int n;
	int err;
	
	err = imxuart_init(sc, 1);
	if (err != 0)
		return err;

	if (0) {
		extern u_int cpufunc_id(void);
		err = cpufunc_id();
		return err;
	}

#if 0
	err = imxuart_snapshot(sc, imxuart_snapshot_data);
	if (err != 0)
		return err;
#endif


	err = imxuart_putchar(sc, 'x');
	if (err != 0)
		return err;

	for (n=100; n--; ) {
		err = imxuart_puts(sc, imxuart_test_string);
		if (err != 0)
			break;
	}

	err = imxuart_snapshot(sc, imxuart_snapshot_data);
	if (err != 0)
		return err;

	return err;
}

static int
imxuart_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct aips_attach_args * const aipsa = aux;

	switch (aipsa->aipsa_addr) {
	case IMX_UART1_BASE:
	case IMX_UART2_BASE:
	case IMX_UART3_BASE:
	case IMX_UART4_BASE:
	case IMX_UART5_BASE:
		return 1;
	default:
		return 0;
	}
}

static void
imxuart_attach(struct device *parent, struct device *self, void *aux)
{
	imxuart_softc_t *sc = (void *)self;
	struct aips_attach_args * const aipsa = aux;
	struct tty *tp;

	sc->sc_bt = aipsa->aipsa_memt;
	sc->sc_addr = aipsa->aipsa_addr;
	sc->sc_size = aipsa->aipsa_size;
	sc->sc_intr = aipsa->aipsa_intr;

	sc->sc_tty = tp = ttymalloc();
	tp->t_oproc = imxuart_start;
	tp->t_param = imxuart_param;
	tty_attach(tp);
	
	shutdownhook_establish(imxuart_shutdownhook, sc);

	printf("\n");
}

void
imxuartcnprobe(struct consdev *cp)
{
	int major;

	__TRACE;
	major = cdevsw_lookup_major(&imxuart_cdevsw);
	cp->cn_dev = makedev(major, 0);			/* XXX unit 0 */
	cp->cn_pri = CN_REMOTE;
}

static void
imxuart_start(struct tty *tp)
{
	__TRACE;
}

static int
imxuart_param(struct tty *tp, struct termios *termios)
{
	__TRACE;
	return 0;
}

static void
imxuart_shutdownhook(void *arg)
{
	__TRACE;
}

void
imxuartcninit(struct consdev *cp)
{
	__TRACE;
}

int
imxuartcngetc(dev_t dev)
{
	struct imxuart_softc *sc;
	uint unit;
	int c;

	unit = IMXUART_UNIT(dev);
	if (unit != 0)
		return 0;
	sc = &imxuart_softc;
	c = imxuart_getchar(sc);
	return c;
}

void
imxuartcnputc(dev_t dev, int c)
{
	(void)imxuart_putchar(&imxuart_softc, c);
}

void
imxuartcnpollc(dev_t dev, int mode)
{
	/* always polled for now */
}
int
imxuart_cnattach(bus_space_tag_t iot, bus_addr_t iobase,
	int rate, int frequency, int type, tcflag_t cflag)
{
	cn_tab = &imxuartcntab;
	return 0;
}


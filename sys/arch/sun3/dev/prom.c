#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/mon.h>

#include <dev/cons.h>
#include "../sun3/interreg.h"

/*
 * cleanup:
 * get autoconfiguration right, right style
 * not a true serial driver but a tty driver, i.e no carrier
 * make sure start is non-blocking
 * add read support via timeouts
 */

void promattach __P((struct device *, struct device *, void *));

struct prom_softc {
    struct device sc_dev;
    int sc_flags;
    int sc_nopen;
    struct tty *sc_tty;
};

struct cfdriver promcd = 
{ NULL, "prom", always_match, promattach, DV_TTY,
      sizeof(struct prom_softc), 0};

#define PROM_CHECK(unit) \
      if (unit >= promcd.cd_ndevs || (promcd.cd_devs[unit] == NULL)) \
	  return ENXIO
#define UNIT_TO_PROM_SC(unit) promcd.cd_devs[unit]
#ifndef PROM_RECEIVE_FREQ
#define PROM_RECEIVE_FREQ 10
#endif    

int promopen __P((dev_t, int, int, struct proc *));
int promclose __P((dev_t, int, int, struct proc *));
int promread __P((dev_t, struct uio *, int));
int promwrite __P((dev_t, struct uio *, int));
int promioctl __P((dev_t, int, caddr_t, int, struct proc *));
int promstop __P((struct tty *, int));

static int promparam __P((struct tty *, struct termios *));
static void promstart __P((struct tty *));
static void promreceive __P((caddr_t));

void promattach(parent, self, args)
     struct device *parent;
     struct device *self;
     void *args;
{
    printf("\n");		
}

int promopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
    struct tty *tp;
    struct prom_softc *prom_sc;
    int unit, s, result;

    unit = minor(dev);
    PROM_CHECK(unit);
    prom_sc = UNIT_TO_PROM_SC(unit);
    if (prom_sc->sc_tty == NULL)
	tp = prom_sc->sc_tty = ttymalloc();
    else
	tp = prom_sc->sc_tty;
    tp->t_addr = (caddr_t) tp;
    tp->t_oproc = promstart;
    tp->t_param = promparam;
    tp->t_dev = dev;
    if ((tp->t_state & TS_ISOPEN) == 0) {
	tp->t_state |= TS_WOPEN;
	ttychars(tp);
	if (tp->t_ispeed == 0) {
	    tp->t_iflag = TTYDEF_IFLAG;
	    tp->t_oflag = TTYDEF_OFLAG;
	    tp->t_cflag = TTYDEF_CFLAG;
	    tp->t_lflag = TTYDEF_LFLAG;
	    tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
	}
	promparam(tp, &tp->t_termios);
	ttsetwater(tp);
    }
    else if (tp->t_state & TS_XCLUDE && p->p_ucred->cr_uid != 0)
	return EBUSY;
    tp->t_state |= TS_CARR_ON;
    result = (*linesw[tp->t_line].l_open)(dev, tp);
    if (result)
	return result;
    timeout((timeout_t) promreceive, (caddr_t) tp, hz/PROM_RECEIVE_FREQ);
    return 0;
}

promclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
    struct tty *tp;
    int unit;
    struct prom_softc *prom_sc;    

    unit = minor(dev);
    prom_sc = UNIT_TO_PROM_SC(unit);
    tp = prom_sc->sc_tty;

    (*linesw[tp->t_line].l_close)(tp, flag);
    return ttyclose(tp);
}

promread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
    int unit;
    register struct tty *tp;
    struct prom_softc *prom_sc;   

    unit = minor(dev);
    prom_sc = UNIT_TO_PROM_SC(unit);

    tp = prom_sc->sc_tty;
    return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}
promwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
    int unit;
    register struct tty *tp;
    struct prom_softc *prom_sc;    

    unit = minor(dev);
    prom_sc = UNIT_TO_PROM_SC(unit);

    tp = prom_sc->sc_tty;
    return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

int promioctl(dev, cmd, data, flag, p)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
        struct proc *p;
{
    int unit;
    register struct tty *tp;
    struct prom_softc *prom_sc;
    int error;

    unit = minor(dev);
    prom_sc = UNIT_TO_PROM_SC(unit);
    tp = prom_sc->sc_tty;

    error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
    if (error >= 0)
	return error;
    error = ttioctl(tp, cmd, data, flag, p);
    if (error >= 0)
	return error;
    return ENOTTY;
}

void promstart(tp)
     struct tty *tp;
{
    int s, c, count;
    u_char outbuf[50];
    u_char *bufp;

    s = spltty();
    if (tp->t_state & (TS_BUSY|TS_TTSTOP|TS_TIMEOUT)) goto out;
    tp->t_state |= TS_BUSY;
    if (tp->t_outq.c_cc <= tp->t_lowat) {
	if (tp->t_state & TS_ASLEEP) {
	    tp->t_state &=~ TS_ASLEEP;
	    wakeup((caddr_t)&tp->t_outq);
	}
	selwakeup(&tp->t_wsel);
    }
    count = q_to_b(&tp->t_outq, outbuf, 49);
    if (count) {
	outbuf[count] = '\0';
	(void) splhigh();
	mon_printf("%s", outbuf);
	(void) spltty();
    }
    tp->t_state &= ~TS_BUSY;
 out:
    splx(s);
}

static int promparam(tp, t)
     struct tty *tp;
     struct termios *t;
{
    struct prom_softc *prom_sc;

    if (!t->c_ispeed || (t->c_ispeed != t->c_ospeed))
	return EINVAL;
    tp->t_ispeed = t->c_ispeed;
    tp->t_ospeed = t->c_ospeed;
    tp->t_cflag = t->c_cflag;
    return 0;
}
static void promreceive(arg)
     caddr_t arg;
{
    struct tty *tp;
    int c, s;

    tp = (struct tty *) arg;
    
    s = spltty();
    if (tp->t_state & TS_ISOPEN) {
	if ((tp->t_state & TS_BUSY) == 0) {
	    extern unsigned int orig_nmi_vector;
	    extern int nmi_intr();
	    
	    set_clk_mode(0,IREG_CLOCK_ENAB_7|IREG_CLOCK_ENAB_5,0);
	    isr_add_custom(7, orig_nmi_vector);
	    set_clk_mode(IREG_CLOCK_ENAB_7,0,1);
	    c = mon_may_getchar();
	    set_clk_mode(0,IREG_CLOCK_ENAB_7|IREG_CLOCK_ENAB_5,0);
	    isr_add_custom(7, nmi_intr);
	    set_clk_mode(IREG_CLOCK_ENAB_5,0,1);
	    if (c != -1) {
		(*linesw[tp->t_line].l_rint)(c, tp);
	    }
	}
	timeout((timeout_t) promreceive, (caddr_t) tp, hz/PROM_RECEIVE_FREQ);
    }
    splx(s);
}



/* prom console support */

promcnprobe(cp)
     struct consdev *cp;
{
    int prommajor;

    /* locate the major number */
    for (prommajor = 0; prommajor < nchrdev; prommajor++)
	if (cdevsw[prommajor].d_open == promopen)
	    break;

    cp->cn_dev = makedev(prommajor, 0);
    cp->cn_pri = CN_INTERNAL;	/* will always exist but you don't
				 * want to use it unless you have to
				 */
}

promcninit(cp)
     struct consdev *cp;
{
    mon_printf("console on prom0\n");
}

promcngetc(dev)
     dev_t dev;
{
    mon_printf("not sure how to do promcngetc() yet\n");
}

/*
 * Console kernel output character routine.
 */
promcnputc(dev, c)
     dev_t dev;
     int c;
{
    int s;

    s = splhigh();
    if (minor(dev) != 0)
	mon_printf("non unit 0 prom console???\n");
    mon_putchar(c);
    splx(s);
}

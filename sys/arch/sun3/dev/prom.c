#include "sys/param.h"
#include "sys/proc.h"
#include "sys/systm.h"
#include "sys/buf.h"
#include "sys/ioctl.h"
#include "sys/tty.h"
#include "sys/file.h"
#include "sys/conf.h"

#include "mon.h"
#include "../sun3/cons.h"

#include "prom.h"

struct prom_softc {
    int flags;
    int nopen;
    struct tty *tp;
} prom_softc[NPROM];

#if NPROM > 1
#error "Can't configure more than one prom tty"
#endif

promopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
    struct tty *tp;
    struct prom_softc *prom;
    int unit;
    int s,error=0;

    unit = UNIT(dev);
    if (unit >= NCOM)
	return ENXIO;
    prom = &prom_softc[unit];
    if (!prom->tp) {
	MALLOC(tp, struct tty *, sizeof(struct tty), M_TTYS, M_WAITOK);
	bzero(tp, sizeof(struct tty));
	prom->tp = tp;
    }
    else tp = prom->tp;
    tp->t_oproc = promstart;
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
	ttsetwater(tp);
	}
    else if (tp->t_state&TS_XCLUDE && p->p_ucred->cr_uid != 0)
	return (EBUSY);
    s = spltty();
    while ((flag&O_NONBLOCK) == 0 && (tp->t_cflag&CLOCAL) == 0 &&
	   (tp->t_state & TS_CARR_ON) == 0) {
	tp->t_state |= TS_WOPEN;
	if (error = ttysleep(tp, (caddr_t)&tp->t_raw, TTIPRI | PCATCH,
			     ttopen, 0))
	    break;
    }
    splx(s);
    if (error == 0)
	error = (*linesw[tp->t_line].l_open)(dev, tp);
    return (error);
}

promclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
    struct tty *tp;

    unit = UNIT(dev);
    if (unit >= NCOM)
	return ENXIO;
    prom = &prom_softc[unit];
    tp = prom->tp;
    (*linesw[tp->t_line].l_close)(tp, flag);
    ttyclose(tp);
    FREE(tp, M_TTYS);
    prom->tp = NULL;
    return 0;
}

promread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp = prom_softc[UNIT(dev)]->tp;
 
	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}
promwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp = prom_softc[UNIT(dev)]->tp;
 
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}


promcnprobe(cp)
     struct consdev *cp;
{

    /* locate the major number */
    for (prommajor = 0; prommajor < nchrdev; prommajor++)
	if (cdevsw[prommajor].d_open == promopen)
	    break;

    cp->cn_dev = makedev(prommajor, 0);
    cp->cn_pri = CN_NORMAL;	/* will always exist but you don't
				 * want to use it unless you have to
				 */
}

static void
promstart(tp)
     struct tty *tp;
{
    int s;

    s = spltty();
    if (tp->t_state & (TS_TIMEOUT | TS_TTSTOP)) {
	splx(s);
	return;
    }
    if (RB_LEN(&tp->t_out) <= tp->t_lowat) {
	if (tp->t_state&TS_ASLEEP) {
	    tp->t_state &= ~TS_ASLEEP;
	    wakeup((caddr_t)&tp->t_out);
	}
	selwakeup(&tp->t_wsel);
    }
    if (RB_LEN(&tp->t_out) == 0)
	goto out;
    c = rbgetc(&tp->t_out);
    tp->t_state |= TS_BUSY;
    mon_putchar(c);
    splx(s);
}

/*
 * Stop output on a line.
 */
promstop(tp, flag)
	register struct tty *tp;
{
	register int s;

	s = spltty();
	if (tp->t_state & TS_BUSY) {
		if ((tp->t_state&TS_TTSTOP)==0)
			tp->t_state |= TS_FLUSH;
	}
	splx(s);
}



promcninit(cp)
     struct consdev *cp;
{
    
}


promcngetc(dev)
     dev_t dev;
{
    mon_printf("not sure how to do promcngetc()\n yet");
}

/*
 * Console kernel output character routine.
 */
promcnputc(c)
	int c;
{
    mon_putchar(c);
}


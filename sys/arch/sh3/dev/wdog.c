#include <sys/param.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/uio.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/syslog.h>

#include <machine/cpu.h>
#include <machine/conf.h>
#include <sh3/shbvar.h>
#include <sh3/wdtreg.h>
#include <sh3/wdogvar.h>

struct wdog_softc {
	struct device sc_dev;		/* generic device structures */
	unsigned int iobase;
	int flags;
};

static int wdogmatch __P((struct device *, struct cfdata *, void *));
static void wdogattach __P((struct device *, struct device *, void *));

struct cfattach wdog_ca = {
	sizeof(struct wdog_softc), wdogmatch, wdogattach
};

extern struct cfdriver wdog_cd;

void
wdog_wr_cnt(x)
	unsigned char x;
{

	SHREG_WTCNT_W = WTCNT_W_M | (unsigned short) x;
}

void
wdog_wr_csr(x)
	unsigned char x;
{

	SHREG_WTCSR_W = WTCSR_W_M | (unsigned short) x;
}

static int
wdogmatch(parent, cfp, aux)
	struct device *parent;
	struct cfdata *cfp;
	void *aux;
{
	struct shb_attach_args *sa = aux;

	if (strcmp(cfp->cf_driver->cd_name, "wdog"))
		return 0;

	sa->ia_iosize = 4;	/* XXX */

	return (1);
}

/*
 *  functions for probeing.
 */
/* ARGSUSED */
static void
wdogattach(parent, self, aux)
	struct device	*parent, *self;
	void		*aux;
{
	struct wdog_softc *sc = (struct wdog_softc *)self;
	struct shb_attach_args *sa = aux;

	sc->iobase = sa->ia_iobase;
	sc->flags = 0;

	printf("\nwdog0: internal watchdog timer\n");
}

/*ARGSUSED*/
int
wdogopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct wdog_softc *sc = wdog_cd.cd_devs[0]; /* XXX */

	if (minor(dev) != 0)
		return (ENXIO);
	if (sc->flags & WDOGF_OPEN)
		return (EBUSY);
	sc->flags |= WDOGF_OPEN;
	return (0);
}

/*ARGSUSED*/
int
wdogclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct wdog_softc *sc = wdog_cd.cd_devs[0]; /* XXX */

	if (sc->flags & WDOGF_OPEN)
		sc->flags = 0;

	return (0);
}

extern unsigned int maxwdog;

/*ARGSUSED*/
int
wdogioctl (dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	int error = 0;
	int request;

	switch (cmd) {
	case SIORESETWDOG:
		wdog_wr_cnt(0);		/* reset to zero */
		break;
	case SIOSTARTWDOG:
		wdog_wr_csr(WTCSR_WT | WTCSR_CKS_4096);
		wdog_wr_cnt(0);		/* reset to zero */
		wdog_wr_csr(SHREG_WTCSR_R | WTCSR_TME);	/* start!!! */
		break;
	case SIOSTOPWDOG:
		wdog_wr_csr(SHREG_WTCSR_R & ~WTCSR_TME); /* stop */
		log(LOG_SYSTEM | LOG_DEBUG, "wdog: maxwdog = %u\n", maxwdog);
		break;
	case SIOSETWDOG:
		request = *(int *)data;

		if (request > 2) {
			error = EINVAL;
			break;
		}
		break;
	default:
		error = EINVAL;
		break;
	}

	return (error);
}

/* $NetBSD: gpioirq.c,v 1.1.36.2 2023/11/26 12:16:31 bouyer Exp $ */

/*
 * Copyright (c) 2016, 2023 Brad Spencer <brad@anduin.eldar.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gpioirq.c,v 1.1.36.2 2023/11/26 12:16:31 bouyer Exp $");

/*
 * GPIO driver that uses interrupts and can send that fact to userland.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/device_impl.h>
#include <sys/gpio.h>
#include <sys/module.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/pool.h>
#include <sys/kmem.h>
#include <sys/condvar.h>
#include <sys/vnode.h>
#include <sys/select.h>
#include <sys/poll.h>

#include <dev/gpio/gpiovar.h>

#define	GPIOIRQ_NPINS		64

struct gpioirq_iv {
	char			sc_intrstr[128];
	void *			sc_ih;
	int			i_thispin_index;
	uint8_t			i_thispin_num;
	uint8_t			i_parentunit;
	struct gpioirq_softc    *sc;
};

struct gpioirq_softc {
	device_t		sc_dev;
	device_t		sc_parentdev;
	void *			sc_gpio;
	struct gpio_pinmap	sc_map;
	int			_map[GPIOIRQ_NPINS];
	struct gpioirq_iv sc_intrs[GPIOIRQ_NPINS];
	int			sc_npins;
	kmutex_t		sc_lock;
	kmutex_t		sc_read_mutex;
	kmutex_t		sc_dying_mutex;
	bool			sc_verbose;
	bool			sc_functional;
	bool			sc_opened;
	bool			sc_dying;
	kcondvar_t              sc_condreadready;
	kcondvar_t		sc_cond_dying;
	pool_cache_t            sc_readpool;
	char                    *sc_readpoolname;
	struct  selinfo 	sc_rsel;
	SIMPLEQ_HEAD(,gpioirq_read_q)  sc_read_queue;
};

struct gpioirq_read_q {
	int	parentunit;
	int	thepin;
	int	theval;
	SIMPLEQ_ENTRY(gpioirq_read_q) read_q;
};

#define	GPIOIRQ_FLAGS_IRQMODE	GPIO_INTR_MODE_MASK
#define	GPIOIRQ_FLAGS_VERBOSE	0x1000

static int	gpioirq_match(device_t, cfdata_t, void *);
static void	gpioirq_attach(device_t, device_t, void *);
static int	gpioirq_detach(device_t, int);
static int	gpioirq_activate(device_t, enum devact);
static int	gpioirq_intr(void *);
static uint8_t	gpioirq_index_to_pin_num(struct gpioirq_softc *, int);
static uint8_t	gpioirq_parent_unit(struct gpioirq_softc *);

CFATTACH_DECL_NEW(gpioirq, sizeof(struct gpioirq_softc),
		  gpioirq_match, gpioirq_attach,
		  gpioirq_detach, gpioirq_activate);

extern struct cfdriver gpioirq_cd;

static dev_type_open(gpioirq_open);
static dev_type_read(gpioirq_read);
static dev_type_close(gpioirq_close);
static dev_type_poll(gpioirq_poll);
const struct cdevsw gpioirq_cdevsw = {
	.d_open = gpioirq_open,
	.d_close = gpioirq_close,
	.d_read = gpioirq_read,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = gpioirq_poll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

static uint8_t
gpioirq_index_to_pin_num(struct gpioirq_softc *sc, int index)
{
	return (uint8_t)gpio_pin_to_pin_num(sc->sc_gpio, &sc->sc_map, index);
}

static uint8_t
gpioirq_parent_unit(struct gpioirq_softc *sc)
{
	device_t parent = sc->sc_parentdev;

	return (uint8_t)parent->dv_unit;
}

static int
gpioirq_match(device_t parent, cfdata_t cf, void *aux)
{
	struct gpio_attach_args *ga = aux;

	if (strcmp(ga->ga_dvname, cf->cf_name))
		return (0);

	if (ga->ga_offset == -1)
		return (0);

	return (1);
}

static void
gpioirq_attach(device_t parent, device_t self, void *aux)
{
	struct gpioirq_softc *sc = device_private(self);
	struct gpio_attach_args *ga = aux;
	int mask = ga->ga_mask;
	int irqmode, flags;

	sc->sc_dev = self;
	sc->sc_parentdev = parent;
	sc->sc_opened = false;
	sc->sc_dying = false;
	sc->sc_readpoolname = NULL;

	/* Map pins */
	sc->sc_gpio = ga->ga_gpio;
	sc->sc_map.pm_map = sc->_map;

	/* Determine our pin configuation. */
	sc->sc_npins = gpio_npins(mask);
	if (sc->sc_npins == 0) {
		sc->sc_npins = 1;
		mask = 0x1;
	}

	/* XXX - exit if more than allowed number of pins */

	if (gpio_pin_map(sc->sc_gpio, ga->ga_offset,
			 mask, &sc->sc_map)) {
		aprint_error(": can't map pins\n");
		return;
	}

	aprint_normal("\n");

	if (ga->ga_flags & GPIOIRQ_FLAGS_VERBOSE)
		sc->sc_verbose = true;

	irqmode = ga->ga_flags & GPIOIRQ_FLAGS_IRQMODE;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);
	mutex_init(&sc->sc_dying_mutex, MUTEX_DEFAULT, IPL_VM);
	mutex_init(&sc->sc_read_mutex, MUTEX_DEFAULT, IPL_VM);
	cv_init(&sc->sc_cond_dying, "girqdie");
	cv_init(&sc->sc_condreadready,"girqrr");
	sc->sc_readpoolname = kmem_asprintf("girqread%d",device_unit(self));
	sc->sc_readpool = pool_cache_init(sizeof(struct gpioirq_read_q),0,0,0,sc->sc_readpoolname,NULL,IPL_VM,NULL,NULL,NULL);
	pool_cache_sethiwat(sc->sc_readpool,100);
	SIMPLEQ_INIT(&sc->sc_read_queue);
	selinit(&sc->sc_rsel);

	for(int apin = 0; apin < sc->sc_npins; apin++) {
		if (!gpio_intr_str(sc->sc_gpio, &sc->sc_map, apin, irqmode,
		    sc->sc_intrs[apin].sc_intrstr, sizeof(sc->sc_intrs[apin].sc_intrstr))) {
			aprint_error_dev(self, "failed to decode interrupt\n");
			return;
		}

		if (!gpio_pin_irqmode_issupported(sc->sc_gpio, &sc->sc_map, apin,
		    irqmode)) {
			aprint_error_dev(self,
			    "irqmode not supported: %s\n", sc->sc_intrs[apin].sc_intrstr);
			gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
			return;
		}

		flags = gpio_pin_get_conf(sc->sc_gpio, &sc->sc_map, apin);
		flags = (flags & ~(GPIO_PIN_OUTPUT|GPIO_PIN_INOUT)) |
		    GPIO_PIN_INPUT;
		if (!gpio_pin_set_conf(sc->sc_gpio, &sc->sc_map, apin, flags)) {
			aprint_error_dev(sc->sc_dev, "pin not capable of input\n");
			gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
			return;
		}

		/* These are static for each pin, so just stuff them in here,
		 * so they don't need to be looked up again.
		 */
		sc->sc_intrs[apin].i_thispin_index = apin;
		sc->sc_intrs[apin].i_thispin_num = gpioirq_index_to_pin_num(sc,apin);
		sc->sc_intrs[apin].i_parentunit = gpioirq_parent_unit(sc);
		sc->sc_intrs[apin].sc = sc;

		sc->sc_intrs[apin].sc_ih = gpio_intr_establish(sc->sc_gpio, &sc->sc_map, apin, IPL_VM,
		    irqmode | GPIO_INTR_MPSAFE,
		    gpioirq_intr, &sc->sc_intrs[apin]);
		if (sc->sc_intrs[apin].sc_ih == NULL) {
			aprint_error_dev(self,
			    "unable to establish interrupt on %s\n", sc->sc_intrs[apin].sc_intrstr);
			gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
			return;
		}
		aprint_normal_dev(self, "interrupting on %s\n", sc->sc_intrs[apin].sc_intrstr);
	}

	sc->sc_functional = true;
}

int
gpioirq_intr(void *arg)
{
	struct gpioirq_iv *is = arg;
	struct gpioirq_softc *sc = is->sc;
	struct gpioirq_read_q *q;
	int val;

	mutex_enter(&sc->sc_lock);

	val = gpio_pin_read(sc->sc_gpio, &sc->sc_map, is->i_thispin_index);

	if (sc->sc_verbose)
		printf("%s: interrupt on %s --> %d\n",
		       device_xname(sc->sc_dev), sc->sc_intrs[is->i_thispin_index].sc_intrstr, val);

	mutex_exit(&sc->sc_lock);

	if (sc->sc_opened) {
		mutex_enter(&sc->sc_read_mutex);
		q = pool_cache_get(sc->sc_readpool,PR_NOWAIT);
		if (q != NULL) {
			q->thepin = is->i_thispin_num;
			q->parentunit = is->i_parentunit;
			q->theval = val;
			SIMPLEQ_INSERT_TAIL(&sc->sc_read_queue,q,read_q);
			selnotify(&sc->sc_rsel, POLLIN|POLLRDNORM, NOTE_SUBMIT);
			cv_signal(&sc->sc_condreadready);
		} else {
			aprint_error("Could not allocate memory for read pool\n");
		}
		mutex_exit(&sc->sc_read_mutex);
	}

	return (1);
}

static int
gpioirq_open(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct gpioirq_softc *sc;

	sc = device_lookup_private(&gpioirq_cd, minor(dev));
	if (!sc)
		return (ENXIO);

	if (sc->sc_opened)
		return (EBUSY);

	mutex_enter(&sc->sc_lock);
	sc->sc_opened = true;
	mutex_exit(&sc->sc_lock);

	return (0);
}

static int
gpioirq_read(dev_t dev, struct uio *uio, int flags)
{
	struct gpioirq_softc *sc;
	struct gpioirq_read_q *chp;
	int error = 0,any;
	uint8_t obuf[3];

	sc = device_lookup_private(&gpioirq_cd, minor(dev));
	if (!sc)
		return (ENXIO);

	if (sc->sc_dying) {
		return EIO;
	}

	while (uio->uio_resid > 0) {
		any = 0;
		error = 0;
		mutex_enter(&sc->sc_read_mutex);

		while (any == 0) {
			chp = SIMPLEQ_FIRST(&sc->sc_read_queue);
			if (chp != NULL) {
				SIMPLEQ_REMOVE_HEAD(&sc->sc_read_queue, read_q);
				any = 1;
				break;
			} else {
				if (flags & IO_NDELAY) {
					error = EWOULDBLOCK;
				} else {
					error = cv_wait_sig(&sc->sc_condreadready,&sc->sc_read_mutex);
				}
				if (sc->sc_dying)
					error = EIO;
				if (error == 0)
					continue;
				break;
			}
		}

		if (any == 1 && error == 0) {
			obuf[0] = (uint8_t)chp->parentunit;
			obuf[1] = (uint8_t)chp->thepin;
			obuf[2] = (uint8_t)chp->theval;
			pool_cache_put(sc->sc_readpool,chp);
			mutex_exit(&sc->sc_read_mutex);
			if ((error = uiomove(&obuf[0], 3, uio)) != 0) {
				break;
			}
		} else {
			mutex_exit(&sc->sc_read_mutex);
			if (error) {
				break;
			}
		}
	}

	if (sc->sc_dying) {
		mutex_enter(&sc->sc_dying_mutex);
		cv_signal(&sc->sc_cond_dying);
		mutex_exit(&sc->sc_dying_mutex);
	}
	return error;
}

static int
gpioirq_close(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct gpioirq_softc *sc;
	struct gpioirq_read_q *q;

	sc = device_lookup_private(&gpioirq_cd, minor(dev));

	if (sc->sc_dying) {
		return(0);
	}

	mutex_enter(&sc->sc_lock);
	while ((q = SIMPLEQ_FIRST(&sc->sc_read_queue)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&sc->sc_read_queue, read_q);
		pool_cache_put(sc->sc_readpool,q);
	}
	sc->sc_opened = false;
	mutex_exit(&sc->sc_lock);

	return(0);
}

static int
gpioirq_poll(dev_t dev, int events, struct lwp *l)
{
        struct gpioirq_softc *sc;
        int revents = 0;

        sc = device_lookup_private(&gpioirq_cd, minor(dev));

	mutex_enter(&sc->sc_read_mutex);
	if (sc->sc_dying) {
                mutex_exit(&sc->sc_read_mutex);
                return POLLHUP;
        }

	if ((events & (POLLIN | POLLRDNORM)) != 0) {
                if (!SIMPLEQ_EMPTY(&sc->sc_read_queue))
                        revents |= events & (POLLIN | POLLRDNORM);
                else
                        selrecord(l, &sc->sc_rsel);
        }

	mutex_exit(&sc->sc_read_mutex);
        return revents;
}

int
gpioirq_detach(device_t self, int flags)
{
	struct gpioirq_softc *sc = device_private(self);
	struct gpioirq_read_q *q;

	/* Clear the handler and disable the interrupt. */
	for(int apin = 0;apin < sc->sc_npins;apin++) {
		gpio_intr_disestablish(sc->sc_gpio, sc->sc_intrs[apin].sc_ih);
	}

	/* Release the pin. */
	gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);

	sc->sc_dying = true;

	if (sc->sc_opened) {
		mutex_enter(&sc->sc_dying_mutex);
		mutex_enter(&sc->sc_read_mutex);
		cv_signal(&sc->sc_condreadready);
		mutex_exit(&sc->sc_read_mutex);
		/* In the worst case this will time out after 5 seconds.
		 * It really should not take that long for the drain / whatever
		 * to happen
		 */
		cv_timedwait_sig(&sc->sc_cond_dying,
		    &sc->sc_dying_mutex, mstohz(5000));
		mutex_exit(&sc->sc_dying_mutex);
		cv_destroy(&sc->sc_condreadready);
		cv_destroy(&sc->sc_cond_dying);
	}

	/* Drain any read pools */
	while ((q = SIMPLEQ_FIRST(&sc->sc_read_queue)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&sc->sc_read_queue, read_q);
		pool_cache_put(sc->sc_readpool,q);
	}

	if (sc->sc_readpoolname != NULL) {
		kmem_free(sc->sc_readpoolname,strlen(sc->sc_readpoolname) + 1);
	}

	mutex_destroy(&sc->sc_read_mutex);
	mutex_destroy(&sc->sc_lock);
	seldestroy(&sc->sc_rsel);

	return (0);
}

int
gpioirq_activate(device_t self, enum devact act)
{

	struct gpioirq_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = true;
		return (0);
	default:
		return (EOPNOTSUPP);
	}
}

MODULE(MODULE_CLASS_DRIVER, gpioirq, "gpio");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
gpioirq_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;
#ifdef _MODULE
	int bmaj = -1, cmaj = -1;
#endif

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_gpioirq,
		    cfattach_ioconf_gpioirq, cfdata_ioconf_gpioirq);
		if (error) {
			aprint_error("%s: unable to init component\n",
			    gpioirq_cd.cd_name);
			return (error);
		}

		error = devsw_attach("gpioirq", NULL, &bmaj,
		    &gpioirq_cdevsw, &cmaj);
		if (error) {
			aprint_error("%s: unable to attach devsw\n",
			    gpioirq_cd.cd_name);
			config_fini_component(cfdriver_ioconf_gpioirq,
			    cfattach_ioconf_gpioirq, cfdata_ioconf_gpioirq);
		}
#endif
		return (error);
	case MODULE_CMD_FINI:
#ifdef _MODULE
		devsw_detach(NULL, &gpioirq_cdevsw);
		config_fini_component(cfdriver_ioconf_gpioirq,
		    cfattach_ioconf_gpioirq, cfdata_ioconf_gpioirq);
#endif
		return (0);
	default:
		return (ENOTTY);
	}
}

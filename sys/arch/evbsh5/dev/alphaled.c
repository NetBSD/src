/*	$NetBSD: alphaled.c,v 1.2 2003/07/14 23:25:41 lukem Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: alphaled.c,v 1.2 2003/07/14 23:25:41 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/fcntl.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <sys/event.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <evbsh5/dev/sysfpgavar.h>
#include <evbsh5/include/alphaledio.h>

static int	alphaled_match(struct device *, struct cfdata *, void *);
static void	alphaled_attach(struct device *, struct device *, void *);

struct alphaled_softc {
	struct device sc_dev;
	bus_space_tag_t sc_bust;
	bus_space_handle_t sc_bush;
	int sc_open;
};

CFATTACH_DECL(alphaled, sizeof(struct alphaled_softc),
    alphaled_match, alphaled_attach, NULL, NULL);
extern struct cfdriver alphaled_cd;


dev_type_open(alphaled_open);
dev_type_close(alphaled_close);
dev_type_write(alphaled_write);
dev_type_ioctl(alphaled_ioctl);

const struct cdevsw alphaled_cdevsw = {
	alphaled_open, alphaled_close,
	noread, alphaled_write,
	alphaled_ioctl,
	nostop, notty, nopoll, nommap, nokqfilter
};

#define	ALPHALED_UNIT(s)	minor(s)


#define	ALPHALED_REG_FLASH(n)		((n) & 7)
#define	ALPHALED_REG_CHAR_RAM(n)	(((n) & 7) + 0x38)
#define	ALPHALED_REG_CTRL		0x30
#define	ALPHALED_REG_SIZE		0x100

#define	ALPHALED_CTRL_BRIGHT(n)		(~(n) & 7)
#define	ALPHALED_CTRL_BRIGHT_MASK	0x7
#define	ALPHALED_CTRL_FLASH		(1u << 3)
#define	ALPHALED_CTRL_BLINK		(1u << 4)
#define	ALPHALED_CTRL_SELF_TEST		(2u << 5)
#define	ALPHALED_CTRL_SELF_TEST_OK	(1u << 5)
#define	ALPHALED_CTRL_RESET		(1u << 7)


#define	alphaled_reg_write(sc, r, v) \
	    bus_space_write_4((sc)->sc_bust, (sc)->sc_bush, \
	    (r) << 2, (u_int32_t)(v) & 0xff)
#define	alphaled_reg_read(sc, r) \
	    (u_int8_t)bus_space_read_4((sc)->sc_bust, (sc)->sc_bush, (r) << 2)

static void alphaled_write_string(struct alphaled_softc *, const char *);

/*ARGSUSED*/
static int
alphaled_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct sysfpga_attach_args *sa = aux;
	struct alphaled_softc sc;
	u_int8_t rv;
	int i;

	if (strcmp(sa->sa_name, alphaled_cd.cd_name) != 0)
		return (0);

	if (bus_space_map(sa->sa_bust, sa->sa_offset, ALPHALED_REG_SIZE, 0,
	    &sc.sc_bush))
		return (0);

	sc.sc_bust = sa->sa_bust;

	alphaled_reg_write(&sc, ALPHALED_REG_CTRL, ALPHALED_CTRL_RESET);
	for (i = 10; i; i--) {
		delay(50);
		rv = alphaled_reg_read(&sc, ALPHALED_REG_CTRL);
		if ((rv & ALPHALED_CTRL_RESET) == 0)
			break;
	}

	bus_space_unmap(sa->sa_bust, sc.sc_bush, ALPHALED_REG_SIZE);

	return (i);
}

/*ARGSUSED*/
static void
alphaled_attach(struct device *parent, struct device *self, void *aux)
{
	struct alphaled_softc *sc = (struct alphaled_softc *)self;
	struct sysfpga_attach_args *sa = aux;

	printf(": Alphanumeric LED display\n");

	sc->sc_bust = sa->sa_bust;
	sc->sc_open = 0;

	/* Map the device. */
	bus_space_map(sc->sc_bust, sa->sa_offset, ALPHALED_REG_SIZE, 0,
	    &sc->sc_bush);
}

/*ARGSUSED*/
int
alphaled_open(dev_t dev, int flag, int mode, struct proc *p)
{
	struct alphaled_softc *sc;

	if ((sc = device_lookup(&alphaled_cd, ALPHALED_UNIT(dev))) == NULL)
		return (ENXIO);

	if (sc->sc_open)
		return (EBUSY);

	if ((flag & FWRITE) == 0)
		return (EPERM);

	sc->sc_open = 1;

	alphaled_write_string(sc, NULL);

	return (0);
}

/*ARGSUSED*/
int
alphaled_close(dev_t dev, int flag, int mode, struct proc *p)
{
	struct alphaled_softc *sc;

	if ((sc = device_lookup(&alphaled_cd, ALPHALED_UNIT(dev))) == NULL)
		return (ENXIO);

	sc->sc_open = 0;
	return (0);
}

/*ARGSUSED*/
int
alphaled_write(dev_t dev, struct uio *uio, int flags)
{
	struct alphaled_softc *sc;
	char buf[ALPHALED_NUM_CHARS];
	size_t n;

	if ((sc = device_lookup(&alphaled_cd, ALPHALED_UNIT(dev))) == NULL)
		return (ENXIO);

	while ((n = min(ALPHALED_NUM_CHARS, uio->uio_resid)) != 0) {
		memset(buf, 0, sizeof(buf));
		if (uiomove(buf, n, uio))
			return (EFAULT);

		alphaled_write_string(sc, buf);
	}

	return (0);
}

/*ARGSUSED*/
int
alphaled_ioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct alphaled_softc *sc;
	u_int8_t rv;
	int d;
	int i;

	if ((sc = device_lookup(&alphaled_cd, ALPHALED_UNIT(dev))) == NULL)
		return (ENXIO);

	switch (cmd) {
	case ALPHALEDIOBRIGHT:
		d = *(int *)data;
		rv = alphaled_reg_read(sc, ALPHALED_REG_CTRL);
		rv &= ~ALPHALED_CTRL_BRIGHT_MASK;
		rv |= ALPHALED_CTRL_BRIGHT(d);
		alphaled_reg_write(sc, ALPHALED_REG_CTRL, rv);
		break;

	case ALPHALEDIOFLASH:
		rv = alphaled_reg_read(sc, ALPHALED_REG_CTRL);
		rv &= ~ALPHALED_CTRL_FLASH;
		d = *(int *)data;
		if ((d & ALPHALED_FLASH_MASK) != 0) {
			rv |= ALPHALED_CTRL_FLASH;
			for (i = 0; i < ALPHALED_NUM_CHARS; i++) {
				alphaled_reg_write(sc, ALPHALED_REG_FLASH(i),
				    (d & (1 << i)) ? 1 : 0);
			}
		}
		alphaled_reg_write(sc, ALPHALED_REG_CTRL, rv);
		break;

	case ALPHALEDIOBLINK:
		rv = alphaled_reg_read(sc, ALPHALED_REG_CTRL);
		d = *(int *)data;
		if (d)
			rv |= ALPHALED_CTRL_BLINK;
		else
			rv &= ~ALPHALED_CTRL_BLINK;
		alphaled_reg_write(sc, ALPHALED_REG_CTRL, rv);
		break;

	default:
		return (ENODEV);
	}

	return (0);
}

static void
alphaled_write_string(struct alphaled_softc *sc, const char *str)
{
	char ch;
	int i;

	if (str) {
		for (i = 0; i < ALPHALED_NUM_CHARS && str[i]; i++) {
			ch = str[i] & 0x7f;
			alphaled_reg_write(sc, ALPHALED_REG_CHAR_RAM(i), ch);
		}
	} else
		i = 0;

	for (; i < ALPHALED_NUM_CHARS; i++)
		alphaled_reg_write(sc, ALPHALED_REG_CHAR_RAM(i), ' ');
}

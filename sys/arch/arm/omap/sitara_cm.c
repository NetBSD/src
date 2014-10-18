/* $NetBSD: sitara_cm.c,v 1.2 2014/10/18 08:33:24 snj Exp $ */
/*
 * Copyright (c) 2010
 *	Ben Gray <ben.r.gray@gmail.com>.
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
 *	This product includes software developed by Ben Gray.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BEN GRAY ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BEN GRAY BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * SCM - System Control Module
 *
 * Hopefully in the end this module will contain a bunch of utility functions
 * for configuring and querying the general system control registers, but for
 * now it only does pin(pad) multiplexing.
 *
 * This is different from the GPIO module in that it is used to configure the
 * pins between modules not just GPIO input/output.
 *
 * This file contains the generic top level driver, however it relies on chip
 * specific settings and therefore expects an array of sitara_cm_padconf structs
 * call ti_padconf_devmap to be located somewhere in the kernel.
 *
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sitara_cm.c,v 1.2 2014/10/18 08:33:24 snj Exp $");

#include "opt_omap.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/proc.h>
#include <sys/kernel.h>

#include <arm/omap/omap2_obiovar.h>
#include <arm/omap/sitara_cm.h>
#include <arm/omap/sitara_cmreg.h>

struct sitara_cm_softc {
        device_t                sc_dev;
	bus_space_tag_t         sc_iot;
	bus_space_handle_t      sc_ioh;
};


static struct sitara_cm_softc *sitara_cm_sc = NULL;

static int	sitara_cm_match(device_t, cfdata_t, void *);
static void	sitara_cm_attach(device_t, device_t, void *);

#define	sitara_cm_read_2(sc, reg)		\
    bus_space_read_2((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define	sitara_cm_write_2(sc, reg, val)		\
    bus_space_write_2((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define	sitara_cm_read_4(sc, reg)		\
    bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define	sitara_cm_write_4(sc, reg, val)		\
    bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))


/**
 *	ti_padconf_devmap - Array of pins, should be defined one per SoC
 *
 *	This array is typically defined in one of the targeted *_scm_pinumx.c
 *	files and is specific to the given SoC platform. Each entry in the array
 *	corresponds to an individual pin.
 */
extern const struct sitara_cm_device sitara_cm_dev;


/**
 *	sitara_cm_padconf_from_name - searches the list of pads and returns entry
 *	                             with matching ball name.
 *	@ballname: the name of the ball
 *
 *	RETURNS:
 *	A pointer to the matching padconf or NULL if the ball wasn't found.
 */
static const struct sitara_cm_padconf*
sitara_cm_padconf_from_name(const char *ballname)
{
	const struct sitara_cm_padconf *padconf;

	padconf = sitara_cm_dev.padconf;
	while (padconf->ballname != NULL) {
		if (strcmp(ballname, padconf->ballname) == 0)
			return(padconf);
		padconf++;
	}
	
	return (NULL);
}

/**
 *	sitara_cm_padconf_set_internal - sets the muxmode and state for a pad/pin
 *	@padconf: pointer to the pad structure
 *	@muxmode: the name of the mode to use for the pin, i.e. "uart1_rx"
 *	@state: the state to put the pad/pin in, i.e. PADCONF_PIN_???
 *	
 *
 *	LOCKING:
 *	Internally locks its own context.
 *
 *	RETURNS:
 *	0 on success.
 *	EINVAL if pin requested is outside valid range or already in use.
 */
static int
sitara_cm_padconf_set_internal(struct sitara_cm_softc *sc,
    const struct sitara_cm_padconf *padconf,
    const char *muxmode, unsigned int state)
{
	unsigned int mode;
	uint16_t reg_val;

	/* populate the new value for the PADCONF register */
	reg_val = (uint16_t)(state & sitara_cm_dev.padconf_sate_mask);

	/* find the new mode requested */
	for (mode = 0; mode < 8; mode++) {
		if ((padconf->muxmodes[mode] != NULL) &&
		    (strcmp(padconf->muxmodes[mode], muxmode) == 0)) {
			break;
		}
	}

	/* couldn't find the mux mode */
	if (mode >= 8) {
		aprint_error_dev(sc->sc_dev, "Invalid mode \"%s\"\n", muxmode);
		return (EINVAL);
	}

	/* set the mux mode */
	reg_val |= (uint16_t)(mode & sitara_cm_dev.padconf_muxmode_mask);
	
	aprint_debug_dev(sc->sc_dev,
	    "setting internal %x for %s\n", reg_val, muxmode);
	/* write the register value (16-bit writes) */
	sitara_cm_write_2(sc, padconf->reg_off, reg_val);
	
	return (0);
}

/**
 *	sitara_cm_padconf_set - sets the muxmode and state for a pad/pin
 *	@padname: the name of the pad, i.e. "c12"
 *	@muxmode: the name of the mode to use for the pin, i.e. "uart1_rx"
 *	@state: the state to put the pad/pin in, i.e. PADCONF_PIN_???
 *	
 *
 *	LOCKING:
 *	Internally locks its own context.
 *
 *	RETURNS:
 *	0 on success.
 *	EINVAL if pin requested is outside valid range or already in use.
 */
int
sitara_cm_padconf_set(const char *padname, const char *muxmode, unsigned int state)
{
	const struct sitara_cm_padconf *padconf;

	if (!sitara_cm_sc)
		return (ENXIO);

	/* find the pin in the devmap */
	padconf = sitara_cm_padconf_from_name(padname);
	if (padconf == NULL)
		return (EINVAL);
	
	return (
	    sitara_cm_padconf_set_internal(sitara_cm_sc, padconf, muxmode, state)
	    );
}

/**
 *	sitara_cm_padconf_get - gets the muxmode and state for a pad/pin
 *	@padname: the name of the pad, i.e. "c12"
 *	@muxmode: upon return will contain the name of the muxmode of the pin
 *	@state: upon return will contain the state of the pad/pin
 *	
 *
 *	LOCKING:
 *	Internally locks its own context.
 *
 *	RETURNS:
 *	0 on success.
 *	EINVAL if pin requested is outside valid range or already in use.
 */
int
sitara_cm_padconf_get(const char *padname, const char **muxmode,
    unsigned int *state)
{
	const struct sitara_cm_padconf *padconf;
	uint16_t reg_val;

	if (!sitara_cm_sc)
		return (ENXIO);

	/* find the pin in the devmap */
	padconf = sitara_cm_padconf_from_name(padname);
	if (padconf == NULL)
		return (EINVAL);
	
	/* read the register value (16-bit reads) */
	reg_val = sitara_cm_read_2(sitara_cm_sc, padconf->reg_off);

	/* save the state */
	if (state)
		*state = (reg_val & sitara_cm_dev.padconf_sate_mask);

	/* save the mode */
	if (muxmode) {
		*muxmode = padconf->muxmodes[
		    (reg_val & sitara_cm_dev.padconf_muxmode_mask)
		    ];
	}
	
	return (0);
}

/**
 *	sitara_cm_padconf_set_gpiomode - converts a pad to GPIO mode.
 *	@gpio: the GPIO pin number (0-195)
 *	@state: the state to put the pad/pin in, i.e. PADCONF_PIN_???
 *
 *	
 *
 *	LOCKING:
 *	Internally locks its own context.
 *
 *	RETURNS:
 *	0 on success.
 *	EINVAL if pin requested is outside valid range or already in use.
 */
int
sitara_cm_padconf_set_gpiomode(uint32_t gpio, unsigned int state)
{
	const struct sitara_cm_padconf *padconf;
	uint16_t reg_val;

	if (!sitara_cm_sc)
		return (ENXIO);
	
	/* find the gpio pin in the padconf array */
	padconf = sitara_cm_dev.padconf;
	while (padconf->ballname != NULL) {
		if (padconf->gpio_pin == gpio)
			break;
		padconf++;
	}
	if (padconf->ballname == NULL)
		return (EINVAL);

	/* populate the new value for the PADCONF register */
	reg_val = (uint16_t)(state & sitara_cm_dev.padconf_sate_mask);

	/* set the mux mode */
	reg_val |=
	    (uint16_t)(padconf->gpio_mode & sitara_cm_dev.padconf_muxmode_mask);

	/* write the register value (16-bit writes) */
	sitara_cm_write_2(sitara_cm_sc, padconf->reg_off, reg_val);

	return (0);
}

/**
 *	sitara_cm_padconf_get_gpiomode - gets the current GPIO mode of the pin
 *	@gpio: the GPIO pin number (0-195)
 *	@state: upon return will contain the state
 *
 *	
 *
 *	LOCKING:
 *	Internally locks its own context.
 *
 *	RETURNS:
 *	0 on success.
 *	EINVAL if pin requested is outside valid range or not configured as GPIO.
 */
int
sitara_cm_padconf_get_gpiomode(uint32_t gpio, unsigned int *state)
{
	const struct sitara_cm_padconf *padconf;
	uint16_t reg_val;

	if (!sitara_cm_sc)
		return (ENXIO);
	
	/* find the gpio pin in the padconf array */
	padconf = sitara_cm_dev.padconf;
	while (padconf->ballname != NULL) {
		if (padconf->gpio_pin == gpio)
			break;
		padconf++;
	}
	if (padconf->ballname == NULL)
		return (EINVAL);

	/* read the current register settings */
	reg_val = sitara_cm_read_2(sitara_cm_sc, padconf->reg_off);
	
	/*
	 * check to make sure the pins is configured as GPIO in the
	 * first state
	 */
	if ((reg_val & sitara_cm_dev.padconf_muxmode_mask) !=
	    padconf->gpio_mode)
		return (EINVAL);
	
	/*
	 * read and store the reset of the state,
	 * i.e. pull-up, pull-down, etc
	 */
	if (state)
		*state = (reg_val & sitara_cm_dev.padconf_sate_mask);
	
	return (0);
}


int
sitara_cm_reg_read_4(uint32_t reg, uint32_t *val)
{
	if (!sitara_cm_sc)
		return (ENXIO);

	*val = sitara_cm_read_4(sitara_cm_sc, reg);
	return (0);
}

int
sitara_cm_reg_write_4(uint32_t reg, uint32_t val)
{
	if (!sitara_cm_sc)
		return (ENXIO);

	sitara_cm_write_4(sitara_cm_sc, reg, val);
	return (0);
}
/*
 * Device part of OMAP SCM driver
 */

CFATTACH_DECL_NEW(sitaracm, sizeof(struct sitara_cm_softc),
    sitara_cm_match, sitara_cm_attach, NULL, NULL);

static int
sitara_cm_match(device_t parent, cfdata_t match, void *opaque)
{
	struct obio_attach_args *obio = opaque;

#ifdef TI_AM335X
	if (obio->obio_addr == 0x44e10000)
		return 1;
#endif
	return 0;
}

static void
sitara_cm_attach(device_t parent, device_t self, void *opaque)
{
	struct sitara_cm_softc *sc = device_private(self);
	struct obio_attach_args *obio = opaque;
	uint32_t rev;

	aprint_naive("\n");

	if (sitara_cm_sc)
		panic("sitara_cm_attach: already attached");

	sc->sc_dev = self;
	sc->sc_iot = obio->obio_iot;

	if (bus_space_map(obio->obio_iot, obio->obio_addr, obio->obio_size,
	    0, &sc->sc_ioh) != 0) {
		aprint_error(": couldn't map address space\n");
		return ;
	}

	sitara_cm_sc = sc;

	if (sitara_cm_reg_read_4(OMAP2SCM_REVISION, &rev) != 0)
		panic("sitara_cm_attach: read revision");
	aprint_normal(": control module, rev %d.%d\n",
	    SCM_REVISION_MAJOR(rev), SCM_REVISION_MINOR(rev));
}

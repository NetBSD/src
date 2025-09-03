/* $NetBSD: drivebay.c,v 1.4 2025/08/18 05:29:04 macallan Exp $ */

/*-
 * Copyright (c) 2025 Michael Lorenz
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * a driver for the Xserve G4's drivebays
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drivebay.c,v 1.4 2025/08/18 05:29:04 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/sysctl.h>

#include <dev/ofw/openfirm.h>
#include <dev/i2c/i2cvar.h>

#ifdef DRIVEBAY_DEBUG
#define DPRINTF printf
#else
#define DPRINTF if (0) printf
#endif

/* commands */
#define PCAGPIO_INPUT	0x00	/* line status */
#define PCAGPIO_OUTPUT	0x01	/* output status */
#define PCAGPIO_REVERT	0x02	/* revert input if set */
#define PCAGPIO_CONFIG	0x03	/* input if set, output if not */

#define O_POWER		0x01
#define O_FAIL		0x02
#define O_INUSE		0x04
#define I_PRESENT	0x08
#define I_SWITCH	0x10
#define I_RESET		0x20
#define I_POWER		0x40
#define I_INUSE		0x80

static int	drivebay_match(device_t, cfdata_t, void *);
static void	drivebay_attach(device_t, device_t, void *);
static int	drivebay_detach(device_t, int);

struct drivebay_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;

	uint32_t	sc_state, sc_input, sc_last_update;

#ifdef DRIVEBAY_DEBUG
	uint32_t	sc_dir, sc_in;
#endif
};


static void 	drivebay_writereg(struct drivebay_softc *, int, uint32_t);
static uint32_t drivebay_readreg(struct drivebay_softc *, int);

static int  sysctl_power(SYSCTLFN_ARGS);
static int  sysctl_fail(SYSCTLFN_ARGS);
static int  sysctl_inuse(SYSCTLFN_ARGS);
static int  sysctl_present(SYSCTLFN_ARGS);
static int  sysctl_switch(SYSCTLFN_ARGS);

CFATTACH_DECL_NEW(drivebay, sizeof(struct drivebay_softc),
    drivebay_match, drivebay_attach, drivebay_detach, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "drivebay-i2c-gpio",	.value = 0 },
	DEVICE_COMPAT_EOL
};

static int
drivebay_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, compat_data, &match_result))
		return 2 * match_result;

	return 0;
}

#ifdef DRIVEBAY_DEBUG
static void
printdir(struct drivebay_softc *sc, uint32_t val, uint32_t mask, char letter)
{
	char flags[17], bits[17];
	uint32_t bit = 0x80;
	int i, cnt = 8;

	val &= mask;
	for (i = 0; i < cnt; i++) {
		flags[i] = (mask & bit) ? letter : '-';
		bits[i] = (val & bit) ? 'X' : ' ';
		bit = bit >> 1;
	}
	flags[cnt] = 0;
	bits[cnt] = 0;
	printf("%s: dir: %s\n", device_xname(sc->sc_dev), flags);
	printf("%s: lvl: %s\n", device_xname(sc->sc_dev), bits);
}	
#endif

static void
drivebay_attach(device_t parent, device_t self, void *aux)
{
	struct drivebay_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	const struct sysctlnode *me = NULL, *node = NULL;
	int ret;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_last_update = 0xffffffff;

	aprint_naive("\n");

	aprint_normal(": PCA9554\n");

	drivebay_writereg(sc, PCAGPIO_CONFIG, 0xf8);
	sc->sc_state = drivebay_readreg(sc, PCAGPIO_OUTPUT);

	ret = sysctl_createv(NULL, 0, NULL, &me,
	    CTLFLAG_READWRITE,
	    CTLTYPE_NODE, device_xname(sc->sc_dev), NULL,
	    NULL, 0, NULL, 0,
	    CTL_HW, CTL_CREATE, CTL_EOL);

	ret = sysctl_createv(NULL, 0, NULL, &node,
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
	    CTLTYPE_INT, "power", "drive power",
	    sysctl_power, 1, (void *)sc, 0,
	    CTL_HW, me->sysctl_num, CTL_CREATE, CTL_EOL);

	ret = sysctl_createv(NULL, 0, NULL, &node,
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
	    CTLTYPE_INT, "fail", "drive fail LED",
	    sysctl_fail, 1, (void *)sc, 0,
	    CTL_HW, me->sysctl_num, CTL_CREATE, CTL_EOL);

	ret = sysctl_createv(NULL, 0, NULL, &node,
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
	    CTLTYPE_INT, "inuse", "drive in use LED",
	    sysctl_inuse, 1, (void *)sc, 0,
	    CTL_HW, me->sysctl_num, CTL_CREATE, CTL_EOL);

	ret = sysctl_createv(NULL, 0, NULL, &node,
	    CTLFLAG_READONLY | CTLFLAG_OWNDESC,
	    CTLTYPE_INT, "present", "drive present",
	    sysctl_present, 1, (void *)sc, 0,
	    CTL_HW, me->sysctl_num, CTL_CREATE, CTL_EOL);

	ret = sysctl_createv(NULL, 0, NULL, &node,
	    CTLFLAG_READONLY | CTLFLAG_OWNDESC,
	    CTLTYPE_INT, "switch", "drive switch",
	    sysctl_switch, 1, (void *)sc, 0,
	    CTL_HW, me->sysctl_num, CTL_CREATE, CTL_EOL);
	__USE(ret);

#ifdef DRIVEBAY_DEBUG
	uint32_t in, out;
	sc->sc_dir = drivebay_readreg(sc, PCAGPIO_CONFIG);
	sc->sc_in = drivebay_readreg(sc, PCAGPIO_INPUT);
	in = sc->sc_in;
	out = sc->sc_state;

	out &= ~sc->sc_dir;
	in &= sc->sc_dir;

	printdir(sc, in, sc->sc_dir, 'I');
	printdir(sc, out, ~sc->sc_dir, 'O');

#endif
}

static int
drivebay_detach(device_t self, int flags)
{
	return 0;
}

static void
drivebay_writereg(struct drivebay_softc *sc, int reg, uint32_t val)
{
	uint8_t cmd;
	uint8_t creg;

	iic_acquire_bus(sc->sc_i2c, 0);
	cmd = reg;
	creg = (uint8_t)val;
	iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP,
		    sc->sc_addr, &cmd, 1, &creg, 1, 0);
	if (reg == PCAGPIO_OUTPUT) sc->sc_state = val;
	iic_release_bus(sc->sc_i2c, 0);
}		

static uint32_t
drivebay_readreg(struct drivebay_softc *sc, int reg)
{
	uint8_t cmd;
	uint8_t creg;
	uint32_t ret;

	if (reg == PCAGPIO_INPUT) {
		if (time_uptime32 == sc->sc_last_update)
			return sc->sc_input;
	}

	iic_acquire_bus(sc->sc_i2c, 0);
	cmd = reg;
	iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
		    sc->sc_addr, &cmd, 1, &creg, 1, 0);
	ret = creg;
	iic_release_bus(sc->sc_i2c, 0);
	if (reg == PCAGPIO_INPUT) {
		sc->sc_last_update = time_uptime32;
		sc->sc_input = ret;
	}
	return ret;
}

static int
sysctl_power(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct drivebay_softc *sc = node.sysctl_data;
	int reg = drivebay_readreg(sc, PCAGPIO_INPUT);
	int power = (reg & I_POWER) != 0;

	if (newp) {
		/* we're asked to write */	
		node.sysctl_data = &power;
		if (sysctl_lookup(SYSCTLFN_CALL(&node)) == 0) {

			power = *(int *)node.sysctl_data;
			reg = sc->sc_state;
			if (power != 0) reg |= O_POWER;
			else reg &= ~O_POWER;
			 			
			if (reg != sc->sc_state) {
				drivebay_writereg(sc, PCAGPIO_OUTPUT, reg);
				sc->sc_state = reg;
			}
			return 0;
		}
		return EINVAL;
	} else {

		node.sysctl_data = &power;
		node.sysctl_size = 4;
		return (sysctl_lookup(SYSCTLFN_CALL(&node)));
	}

	return 0;
}

static int
sysctl_inuse(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct drivebay_softc *sc = node.sysctl_data;
	int reg = drivebay_readreg(sc, PCAGPIO_INPUT);
	int inuse = (reg & I_INUSE) != 0;

	if (newp) {
		/* we're asked to write */	
		node.sysctl_data = &inuse;
		if (sysctl_lookup(SYSCTLFN_CALL(&node)) == 0) {

			inuse = *(int *)node.sysctl_data;
			reg = sc->sc_state;
			if (inuse != 0) reg |= O_INUSE;
			else reg &= ~O_INUSE;
			 			
			if (reg != sc->sc_state) {
				drivebay_writereg(sc, PCAGPIO_OUTPUT, reg);
				sc->sc_state = reg;
			}
			return 0;
		}
		return EINVAL;
	} else {

		node.sysctl_data = &inuse;
		node.sysctl_size = 4;
		return (sysctl_lookup(SYSCTLFN_CALL(&node)));
	}

	return 0;
}

static int
sysctl_fail(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct drivebay_softc *sc = node.sysctl_data;
	int reg = sc->sc_state;
	int fail = (reg & O_FAIL) != 0;

	if (newp) {
		/* we're asked to write */	
		node.sysctl_data = &fail;
		if (sysctl_lookup(SYSCTLFN_CALL(&node)) == 0) {

			fail = *(int *)node.sysctl_data;
			reg = sc->sc_state;
			if (fail != 0) reg |= O_FAIL;
			else reg &= ~O_FAIL;
			 			
			if (reg != sc->sc_state) {
				drivebay_writereg(sc, PCAGPIO_OUTPUT, reg);
				sc->sc_state = reg;
			}
			return 0;
		}
		return EINVAL;
	} else {

		node.sysctl_data = &fail;
		node.sysctl_size = 4;
		return (sysctl_lookup(SYSCTLFN_CALL(&node)));
	}

	return 0;
}

static int
sysctl_present(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct drivebay_softc *sc = node.sysctl_data;
	int reg = drivebay_readreg(sc, PCAGPIO_INPUT);
	int present = (reg & I_PRESENT) == 0;

	if (newp) {
		/* we're asked to write */	
		return EINVAL;
	} else {

		node.sysctl_data = &present;
		node.sysctl_size = 4;
		return (sysctl_lookup(SYSCTLFN_CALL(&node)));
	}

	return 0;
}

static int
sysctl_switch(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct drivebay_softc *sc = node.sysctl_data;
	int reg = drivebay_readreg(sc, PCAGPIO_INPUT);
	int sw = (reg & I_SWITCH) == 0;

	if (newp) {
		/* we're asked to write */	
		return EINVAL;
	} else {

		node.sysctl_data = &sw;
		node.sysctl_size = 4;
		return (sysctl_lookup(SYSCTLFN_CALL(&node)));
	}

	return 0;
}

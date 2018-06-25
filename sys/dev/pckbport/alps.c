/* $NetBSD: alps.c,v 1.4.6.1 2018/06/25 07:26:01 pgoyette Exp $ */

/*-
 * Copyright (c) 2017 Ryo ONODERA <ryo@tetera.org>
 * Copyright (c) 2008 Jared D. McNeill <jmcneill@invisible.ca>
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

#include "opt_pms.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: alps.c,v 1.4.6.1 2018/06/25 07:26:01 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/bus.h>
#include <sys/bitops.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#include <dev/pckbport/pckbportvar.h>
#include <dev/pckbport/pmsreg.h>
#include <dev/pckbport/pmsvar.h>
#include <dev/pckbport/alpsreg.h>
#include <dev/pckbport/alpsvar.h>

/* #define ALPS_DEBUG */

static int alps_touchpad_xy_unprecision_nodenum;
static int alps_trackstick_xy_precision_nodenum;

static int alps_touchpad_xy_unprecision = 2;
static int alps_trackstick_xy_precision = 1;

static void pms_alps_input_v7(void *, int);
static void pms_alps_input_v2(void *, int);

static int
pms_sysctl_alps_verify(SYSCTLFN_ARGS)
{
	int error, t;
	struct sysctlnode node;

	node = *rnode;
	t = *(int *)rnode->sysctl_data;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (node.sysctl_num == alps_touchpad_xy_unprecision_nodenum ||
		node.sysctl_num == alps_trackstick_xy_precision_nodenum) {
		if (t < 0 || t > 7)
			return EINVAL;
	} else
		return EINVAL;

	*(int *)rnode->sysctl_data = t;

	return 0;

}

static void
pms_sysctl_alps(struct sysctllog **clog)
{
	const struct sysctlnode *node;
	int rc, root_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
		CTLFLAG_PERMANENT, CTLTYPE_NODE, "alps",
		SYSCTL_DESCR("ALPS touchpad controls"),
		NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	root_num = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
		CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		CTLTYPE_INT, "touchpad_xy_precision_shift",
		SYSCTL_DESCR("Touchpad X/Y-axis precision shift value"),
		pms_sysctl_alps_verify, 0,
		&alps_touchpad_xy_unprecision,
		0, CTL_HW, root_num, CTL_CREATE,
		CTL_EOL)) != 0)
			goto err;
	alps_touchpad_xy_unprecision_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
		CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		CTLTYPE_INT, "tackstick_xy_precision_shift",
		SYSCTL_DESCR("Trackstick X/Y-axis precision value"),
		pms_sysctl_alps_verify, 0,
		&alps_trackstick_xy_precision,
		0, CTL_HW, root_num, CTL_CREATE,
		CTL_EOL)) != 0)
			goto err;
	alps_trackstick_xy_precision_nodenum = node->sysctl_num;

	return;

err:
	aprint_error("%s: sysctl_createv failed (rc = %d)\n",
		__func__, rc);
}

/*
 * Publish E6 report command and get E6 signature,
 * then check the signature
 */
static int
pms_alps_e6sig(struct pms_softc *psc, uint8_t *e6sig)
{
	uint8_t cmd[2];
	int res;

	e6sig[0] = 0;
	cmd[0] = PMS_SET_RES; /* E8 */
	cmd[1] = 0;
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 2, 0, NULL, 0)) != 0)
		goto err;
	cmd[0] = PMS_SET_SCALE11; /* E6 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;
	cmd[0] = PMS_SET_SCALE11; /* E6 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;
	cmd[0] = PMS_SET_SCALE11; /* E6 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;
	e6sig[0] = e6sig[1] = e6sig[2] = 0;
	/* Get E6 signature */
	cmd[0] = PMS_SEND_DEV_STATUS; /* E9 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 3, e6sig, 0)) != 0)
		goto err;

	/* ALPS input device returns 00-00-64 as E6 signature */
	if ((e6sig[0] & ~0x07u) != 0x00 || /* ignore buttons */
	    e6sig[1] != 0x00 ||
	    e6sig[2] != 0x64)
	{
		return ENODEV;	/* This is not an ALPS device */
	}

	aprint_debug_dev(psc->sc_dev,
		"ALPS PS/2 E6 signature: 0x%X 0x%X 0x%X\n",
		e6sig[0], e6sig[1], e6sig[2]);

	return 0;
err:
	aprint_error_dev(psc->sc_dev, "Failed to get E6 signature.\n");
	return res;
}

/*
 * Publish E7 report command and get E7 signature
 */
static int
pms_alps_e7sig(struct pms_softc *psc, uint8_t *e7sig)
{
	uint8_t cmd[2];
	int res;

	cmd[0] = PMS_SET_RES; /* E8 */
	cmd[1] = 0;
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 2, 0, NULL, 0)) != 0)
		goto err;
	cmd[0] = PMS_SET_SCALE21; /* E7 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;
	cmd[0] = PMS_SET_SCALE21; /* E7 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;
	cmd[0] = PMS_SET_SCALE21; /* E7 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;
	e7sig[0] = e7sig[1] = e7sig[2] = 0;
	cmd[0] = PMS_SEND_DEV_STATUS; /* E9 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 3, e7sig, 0)) != 0)
		goto err;

	aprint_debug_dev(psc->sc_dev,
		"ALPS PS/2 E7 signature: 0x%X 0x%X 0x%X\n",
		e7sig[0], e7sig[1], e7sig[2]);

	if (e7sig[0] != 0x73)
		return ENODEV;	/* This is not an ALPS device */

	return 0;
err:
	aprint_error_dev(psc->sc_dev, "Failed to get E7 signature.\n");
	return res;
}

/*
 * Publish EC command and get EC signature
 */
static int
pms_alps_ecsig(struct pms_softc *psc, uint8_t *ecsig)
{
	uint8_t cmd[2];
	int res;

	cmd[0] = PMS_SET_RES; /* E8 */
	cmd[1] = 0;
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 2, 0, NULL, 0)) != 0)
		goto err;
	cmd[0] = PMS_RESET_WRAP_MODE; /* EC */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;
	cmd[0] = PMS_RESET_WRAP_MODE; /* EC */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;
	cmd[0] = PMS_RESET_WRAP_MODE; /* EC */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;
	ecsig[0] = ecsig[1] = ecsig[2] = 0;
	cmd[0] = PMS_SEND_DEV_STATUS; /* E9 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 3, ecsig, 0)) != 0)
		goto err;

	aprint_debug_dev(psc->sc_dev,
		"ALPS PS/2 EC signature: 0x%X 0x%X 0x%X\n",
		ecsig[0], ecsig[1], ecsig[2]);

	return 0;

err:
	aprint_debug_dev(psc->sc_dev, "Failed to get EC signature.\n");
	return res;
}

/*
 * Enter to command mode
 */
static int
pms_alps_start_command_mode(struct pms_softc *psc)
{
	uint8_t cmd[1];
	uint8_t resp[3];
	int res;

	cmd[0] = PMS_RESET_WRAP_MODE; /* EC */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;
	cmd[0] = PMS_RESET_WRAP_MODE; /* EC */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;
	cmd[0] = PMS_RESET_WRAP_MODE; /* EC */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;
	resp[0] = resp[1] = resp[2] = 0;
	cmd[0] = PMS_SEND_DEV_STATUS; /* E9 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 3, resp, 0)) != 0)
		goto err;

	aprint_debug_dev(psc->sc_dev, "ALPS Firmware ID: 0x%x 0x%X 0x%X\n",
		resp[0], resp[1], resp[2]);

	if (resp[0] != 0x88 || (resp[1] & __BITS(7, 4)) != 0xb0)
		return EINVAL;

	return 0;
err:
	aprint_error_dev(psc->sc_dev, "Failed to start command mode.\n");
	return res;
}

/*
 * End command mode
 */
static int
pms_alps_end_command_mode(struct pms_softc *psc)
{
	int res;
	uint8_t cmd[1];

	cmd[0] = PMS_SET_STREAM; /* EA */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;

	return res;

err:
	aprint_error_dev(psc->sc_dev, "Failed to end command mode.\n");
	return res;
}

/*
 * Write nibble (4-bit) data
 */
static int
pms_alps_cm_write_nibble(pckbport_tag_t tag, pckbport_slot_t slot, uint8_t nibble)
{
	uint8_t cmd[2];
	uint8_t resp[3];
	int sendparam;
	int recieve;
	int res;

	sendparam = alps_v7_nibble_command_data_arr[nibble].sendparam;
	recieve= alps_v7_nibble_command_data_arr[nibble].recieve;
	cmd[0] = alps_v7_nibble_command_data_arr[nibble].command;
	if (recieve) {
		if ((res = pckbport_poll_cmd(tag, slot, cmd, 1, 3, resp, 0)) != 0) {
			aprint_error("send nibble error: %d\n", res);
		}
	} else if (sendparam) {
		cmd[1] = alps_v7_nibble_command_data_arr[nibble].data;
		if ((res = pckbport_poll_cmd(tag, slot, cmd, 2, 0, NULL, 0)) != 0) {
			aprint_error("send nibble error: %d\n", res);
		}
	} else {
		if ((res = pckbport_poll_cmd(tag, slot, cmd, 1, 0, NULL, 0)) != 0) {
			aprint_error("send nibble error: %d\n", res);
		}
	}

	return res;
}

/*
 * Set an register address for read and write
 */
static int
pms_alps_set_address(pckbport_tag_t tag, pckbport_slot_t slot, uint16_t reg)
{
	uint8_t cmd[1];
	uint8_t nibble;
	int res;

	cmd[0] = PMS_RESET_WRAP_MODE;
	if ((res = pckbport_poll_cmd(tag, slot, cmd, 1, 0, NULL, 0)) != 0)
		goto err;

	/* Set address */
	nibble = (reg >> 12) & __BITS(3, 0);
	if ((res = pms_alps_cm_write_nibble(tag, slot, nibble)) != 0) {
		goto err;
	}
	nibble = (reg >> 8) & __BITS(3, 0);
	if ((res = pms_alps_cm_write_nibble(tag, slot, nibble)) != 0) {
		goto err;
	}
	nibble = (reg >> 4) & __BITS(3, 0);
	if ((res = pms_alps_cm_write_nibble(tag, slot, nibble)) != 0) {
		goto err;
	}
	nibble = (reg >> 0) & __BITS(3, 0);
	if ((res = pms_alps_cm_write_nibble(tag, slot, nibble)) != 0) {
		goto err;
	}
	return res;

err:
	aprint_error("Failed to set addess.\n");
	return res;
}

/*
 * Read one byte from register
 */
static int
pms_alps_cm_read_1(pckbport_tag_t tag, pckbport_slot_t slot, uint16_t reg,
	uint8_t *val)
{
	uint8_t cmd[1];
	uint8_t resp[3];
	int res;

	if ((res = pms_alps_set_address(tag, slot, reg)) != 0)
		goto err;

	cmd[0] = PMS_SEND_DEV_STATUS;
	if ((res = pckbport_poll_cmd(tag, slot,
	    cmd, 1, 3, resp, 0)) != 0) {
		goto err;
	}

	if (reg != ((resp[0] << 8) | resp[1])) {
		return EINVAL;
	}

	*val = resp[2];
	return res;

err:
	aprint_error("Failed to read a value.\n");
	*val = 0;
	return res;
}

/*
 * Write one byte to register
 */
static int
pms_alps_cm_write_1(pckbport_tag_t tag, pckbport_slot_t slot, uint16_t reg,
	uint8_t val)
{
	uint8_t nibble;
	int res;

	if ((res = pms_alps_set_address(tag, slot, reg)) != 0)
		goto err;

	nibble = __SHIFTOUT(val, __BITS(7, 4));
	if ((res = pms_alps_cm_write_nibble(tag, slot, nibble)) != 0) {
		goto err;
	}

	nibble = __SHIFTOUT(val, __BITS(3, 0));
	if ((res = pms_alps_cm_write_nibble(tag, slot, nibble)) != 0) {
		goto err;
	}

	return res;
err:
	aprint_error("Failed to write a value.\n");
	return res;
}

/*
 * Not used practically for initialization
 */
static int
pms_alps_get_resolution_v7(struct pms_softc *psc)
{
#if 0
	struct alps_softc *sc = &psc->u.alps;
#endif
	pckbport_tag_t tag = psc->sc_kbctag;
	pckbport_slot_t slot = psc->sc_kbcslot;

	int res;
	uint8_t ret;
#if 0
	uint32_t x_pitch, y_pitch;
	uint32_t x_elec, y_elec;
	uint32_t x_phy, y_phy;
#endif
	/* X/Y pitch */
	if ((res = pms_alps_cm_read_1(tag, slot, 0xc397, &ret)) != 0) {
		goto err;
	}
#if 0
	/* X pitch */
	x_pitch = __SHIFTOUT(ret, __BITS(7, 4)); /* Higher 4-bit */
	x_pitch = x_pitch * 2 + 50; /* Unit = 0.1mm */

	/* Y pitch */
	y_pitch = ret & __BITS(3, 0); /* Lower 4-bit */
	y_pitch = y_pitch * 2 + 36; /* Unit = 0.1mm */

	/* X/Y electrode */
	if ((res = pms_alps_cm_read_1(tag, slot, 0xc397 + 1, &ret)) != 0) {
		goto err;
	}

	/* X electrode */
	x_elec = __SHIFTOUT(ret, __BITS(7, 4)); /* Higher 4-bit */
	x_elec = x_elec + 17;

	/* Y electrode */
	y_elec = ret & __BITS(3, 0); /* Lower 4-bit */
	y_elec = y_elec + 13;

	/* X/Y physical in unit = 0.1mm */
	/* X physical */
	x_phy = (x_elec - 1) * x_pitch;
	y_phy = (y_elec - 1) * y_pitch;

	/* X/Y resolution (unit) */
	sc->res_x = 0xfff * 10 / x_phy;
	sc->res_y = 0x7ff * 10 / y_phy;
#endif
	return res;

err:
	aprint_error("Failed to get resolution.\n");
	return res;
}

/*
 * Enable tap mode for V2 device
 */
static int
pms_alps_enable_tap_mode_v2(struct pms_softc *psc)
{
	uint8_t cmd[2];
	uint8_t resp[3];
	int res;

	resp[0] = resp[1] = resp[2] = 0;
	cmd[0] = PMS_SEND_DEV_STATUS; /* E9 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 3, resp, 0)) != 0)
		goto err;
	cmd[0] = PMS_DEV_DISABLE; /* F5 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;
	cmd[0] = PMS_DEV_DISABLE; /* F5 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;
	cmd[0] = PMS_SET_SAMPLE;
	cmd[1] = 0x0a; /* argument */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 2, 0, NULL, 0)) != 0)
		goto err;

	/* Get status */
	cmd[0] = PMS_DEV_DISABLE; /* F5 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;
	cmd[0] = PMS_DEV_DISABLE; /* F5 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;
	cmd[0] = PMS_DEV_DISABLE; /* F5 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;
	resp[0] = resp[1] = resp[2] = 0;
	cmd[0] = PMS_SEND_DEV_STATUS; /* E9 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 3, resp, 0)) != 0)
		goto err;

	aprint_debug_dev(psc->sc_dev, "Tap mode is enabled.\n");

	return 0;
err:
	aprint_error_dev(psc->sc_dev, "Failed to enable tap mode.\n");
	return res;
}

static int
pms_alps_init_v2(struct pms_softc *psc)
{
	uint8_t cmd[1];
	int res;

	if ((res = pms_alps_enable_tap_mode_v2(psc)) != 0)
		goto err;

	/* Enable absolute mode */
	cmd[0] = PMS_DEV_DISABLE; /* F5 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;
	cmd[0] = PMS_DEV_DISABLE; /* F5 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;
	cmd[0] = PMS_DEV_DISABLE; /* F5 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;
	cmd[0] = PMS_DEV_DISABLE; /* F5 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;
	cmd[0] = PMS_DEV_ENABLE; /* F4 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;

	/* Enable remote mode */
	cmd[0] = PMS_SET_REMOTE_MODE; /* F0 */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;

	/* Start stream mode to get data */
	cmd[0] = PMS_SET_STREAM; /* EA */
	if ((res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 0, NULL, 0)) != 0)
		goto err;


	return 0;

err:
	aprint_error_dev(psc->sc_dev, "Failed to initialize V2 device.\n");
	return res;
}

static int
pms_alps_init_v7(struct pms_softc *psc)
{
	uint8_t val;
	uint8_t nibble;
	int res;

	/* Start command mode */
	if ((res = pms_alps_start_command_mode(psc)) != 0) {
		goto err;
	}
	if ((res = pms_alps_cm_read_1(psc->sc_kbctag, psc->sc_kbcslot, 0xc2d9, &val)) != 0) {
		goto err;
	}
	if ((res = pms_alps_get_resolution_v7(psc)) != 0) {
		goto err;
	}
	if ((res = pms_alps_cm_write_1(psc->sc_kbctag, psc->sc_kbcslot, 0xc2c9, 0x64)) != 0) {
		goto err;
	}

	/* Start absolute mode */
	if ((res = pms_alps_cm_read_1(psc->sc_kbctag, psc->sc_kbcslot, 0xc2c4, &val)) != 0) {
		goto err;
	}
	/* Do not set address before this, so do not use pms_cm_write_1() */
	val = val | __BIT(1);
	nibble = __SHIFTOUT(val, __BITS(7, 4));
	if ((res = pms_alps_cm_write_nibble(psc->sc_kbctag, psc->sc_kbcslot, nibble)) != 0) {
		goto err;
	}
	nibble = __SHIFTOUT(val, __BITS(3, 0));
	if ((res = pms_alps_cm_write_nibble(psc->sc_kbctag, psc->sc_kbcslot, nibble)) != 0) {
		goto err;
	}

	/* End command mode */
	if ((res = pms_alps_end_command_mode(psc)) != 0)
		goto err;

	return res;

err:
	(void)pms_alps_end_command_mode(psc);
	aprint_error_dev(psc->sc_dev, "Failed to initialize V7 device.\n");
	return res;
}

int
pms_alps_probe_init(void *opaque)
{
	struct pms_softc *psc = opaque;
	struct alps_softc *sc = &psc->u.alps;
	struct sysctllog *clog = NULL;
	uint8_t e6sig[3];
	uint8_t e7sig[3];
	uint8_t ecsig[3];
	int res;
	u_char cmd[1];

	sc->last_x1 = 0;
	sc->last_y1 = 0;
	sc->last_x2 = 0;
	sc->last_y2 = 0;
	sc->last_nfingers = 0;

	pckbport_flush(psc->sc_kbctag, psc->sc_kbcslot);

	if ((res = pms_alps_e6sig(psc, e6sig)) != 0)
		goto err;

	if ((res = pms_alps_e7sig(psc, e7sig)) != 0)
		goto err;

	if ((res = pms_alps_ecsig(psc, ecsig)) != 0)
		goto err;

	/* Determine protocol version */
	if ((ecsig[0] == 0x88) && (__SHIFTOUT(ecsig[1], __BITS(7, 4)) == 0x0b)) {
		/* V7 device in Toshiba dynabook R63/PS */
		sc->version = ALPS_PROTO_V7;
	} else if ((e7sig[0] == 0x73) && (e7sig[1] == 0x02) &&
		(e7sig[2] == 0x14)) {
		/* V2 device in NEC VJ22MF-7 (VersaPro JVF-7) */
		sc->version = ALPS_PROTO_V2;
	}

	if (sc->version == ALPS_PROTO_V7) {
		/* Initialize V7 device */
		if ((res = pms_alps_init_v7(psc)) != 0)
			goto err;
		aprint_normal_dev(psc->sc_dev,
			"ALPS PS/2 V7 pointing device\n");
	} else if (sc->version == ALPS_PROTO_V2) {
		/* Initialize V2 pointing device */
		if ((res = pms_alps_init_v2(psc)) != 0)
			goto err;
		aprint_normal_dev(psc->sc_dev,
			"ALPS PS/2 V2 pointing device\n");
	} else {
		res = EINVAL;
		goto err;
	}

	/* From sysctl */
	pms_sysctl_alps(&clog);
	/* Register hundler */
	if (sc->version == ALPS_PROTO_V7) {
		pckbport_set_inputhandler(psc->sc_kbctag, psc->sc_kbcslot,
			pms_alps_input_v7, psc, device_xname(psc->sc_dev));
	} else if (sc->version == ALPS_PROTO_V2) {
		pckbport_set_inputhandler(psc->sc_kbctag, psc->sc_kbcslot,
			pms_alps_input_v2, psc, device_xname(psc->sc_dev));
	} else {
		res = EINVAL;
		goto err;
	}
	/* Palm detection is enabled. */

	return 0;

err:
	cmd[0] = PMS_RESET;
	(void)pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 2, NULL, 1);
	if (res != ENODEV)
		aprint_error_dev(psc->sc_dev, "Failed to initialize an ALPS device.\n");
	return res;
}

void
pms_alps_enable(void *opaque)
{
	struct pms_softc *psc = opaque;
	struct alps_softc *sc = &psc->u.alps;

	sc->initializing = true;
}

void
pms_alps_resume(void *opaque)
{
	struct pms_softc *psc = opaque;
	struct alps_softc *sc = &psc->u.alps;
	uint8_t cmd, resp[2];
	int res;

	cmd = PMS_RESET;
	res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot, &cmd,
		1, 2, resp, 1);
	if (res)
	aprint_error_dev(psc->sc_dev,
		"ALPS reset on resume failed\n");
	else {
		if (sc->version == ALPS_PROTO_V7) {
			(void)pms_alps_init_v7(psc);
		} else if (sc->version == ALPS_PROTO_V2) {
			(void)pms_alps_init_v2(psc);
		} else {
			/* Not supported */
		}
		pms_alps_enable(psc);
	}
}

static void
pms_alps_decode_trackstick_packet_v7(struct pms_softc *psc)
{
	int s;

	int x, y, z;
	int dx, dy, dz;
	int left, middle, right;
	u_int buttons;

	x = (int8_t)((psc->packet[2] & 0xbf) |
		((psc->packet[3] & 0x10) << 2));
	y = (int8_t)((psc->packet[3] & 0x07) | (psc->packet[4] & 0xb8) |
		((psc->packet[3] & 0x20) << 1));
	z = (int8_t)((psc->packet[5] & 0x3f) |
		((psc->packet[3] & 0x80) >> 1));

	dx = x * alps_trackstick_xy_precision;
	dy = y * alps_trackstick_xy_precision;
	dz = z * 1;

	left = psc->packet[1] & 0x01;
	middle = (psc->packet[1] & 0x04) >> 2;
	right = (psc->packet[1] & 0x02) >> 1;
	buttons = 0;
	buttons = (u_int)((left << 0) | (middle << 1) | (right << 2));

	s = spltty();
	wsmouse_input(psc->sc_wsmousedev,
		buttons,
		dx, dy, dz, 0,
		WSMOUSE_INPUT_DELTA);
	splx(s);
}

static uint8_t
pms_alps_decode_packetid_v7(struct pms_softc *psc)
{
	if (psc->packet[4] & 0x40)
		return ALPS_V7_PACKETID_TWOFINGER;
	else if (psc->packet[4] & 0x01)
		return ALPS_V7_PACKETID_MULTIFINGER;
	else if ((psc->packet[0] & 0x10) && !(psc->packet[4] & 0x43))
		return ALPS_V7_PACKETID_NEWPACKET;
	else if ((psc->packet[1] == 0x00) && (psc->packet[4] == 0x00))
		return ALPS_V7_PACKETID_IDLE;
	else
		return ALPS_V7_PACKETID_UNKNOWN;
}

static void
pms_alps_decode_touchpad_packet_v7(struct pms_softc *psc)
{
	int s;
	struct alps_softc *sc = &psc->u.alps;
	uint8_t packetid;

	uint16_t cur_x1, cur_y1;
	uint16_t cur_x2, cur_y2;
	int dx1, dy1;
	int button;
	u_int buttons;

	packetid = pms_alps_decode_packetid_v7(psc);
	switch (packetid) {
	case ALPS_V7_PACKETID_IDLE:
		/* Accept meaningful packets only */
		return;
	case ALPS_V7_PACKETID_UNKNOWN:
		/* Accept meaningful packets only */
		return;
	case ALPS_V7_PACKETID_NEWPACKET:
		/* Sent new packet ID to reset status and not decoded */
		sc->initializing = true;
		return;
	}

	/* Decode a number of fingers and locations */
	/* X0-11 ... X0-0 */
	cur_x1 = (psc->packet[2] & 0x80) << 4; /* X0-11 */
	cur_x1 |= (psc->packet[2] & 0x3f) << 5; /* X0-10 ... X0-5 */
	cur_x1 |= (psc->packet[3] & 0x30) >> 1; /* X0-4, X0-3 */
	cur_x1 |= psc->packet[3] & 0x07; /* X0-2 ... X0-0 */

	/* Y0-10 ... Y0-0 */
	cur_y1 = psc->packet[1] << 3; /* Y0-10 ... Y0-3 */
	cur_y1 |= psc->packet[0] & 0x07; /* Y0-2 ... Y0-0 */

	/* X1-11 ... X1-3 */
	cur_x2 = (psc->packet[3] & 0x80) << 4; /* X1-11 */
	cur_x2 |= (psc->packet[4] & 0x80) << 3; /* X1-10 */
	cur_x2 |= (psc->packet[4] & 0x3f) << 4; /* X1-9 ... X1-4 */

	/* Y1-10 ... Y1-4 */
	cur_y2 = (psc->packet[5] & 0x80) << 3; /* Y1-10 */
	cur_y2 |= (psc->packet[5] & 0x3f) << 4; /* Y1-9 .. Y1-4 */

	switch (packetid) {
	case ALPS_V7_PACKETID_TWOFINGER:
		cur_x2 &= ~__BITS(3, 0); /* Clear undefined locations */
		cur_y2 |= __BITS(3, 0); /* Fill undefined locations */
		break;
	case ALPS_V7_PACKETID_MULTIFINGER:
		cur_x2 &= ~__BITS(5, 0); /* Clear undefined locations */
		cur_y2 &= ~__BIT(5); /* Clear duplicate locations */
		cur_y2 |= (psc->packet[4] & __BIT(1)) << 4; /* Set another */
		cur_y2 |= __BITS(4, 0); /* Fill undefined locations */
		break;
	}

	cur_y1 = 0x7ff - cur_y1;
	cur_y2 = 0x7ff - cur_y2;

	/* Handle finger touch reported in cur_x2/y2. only */
	if (cur_x1 == 0 && cur_y1 == 0 && cur_x2 != 0 && cur_y2 != 0) {
		cur_x1 = cur_x2;
		cur_y1 = cur_y2;
		cur_x2 = 0;
		cur_y2 = 0;
	}

	switch (packetid) {
	case ALPS_V7_PACKETID_TWOFINGER:
		if ((cur_x2 == 0) && (cur_y2 == 0))
			sc->nfingers = 1;
		else
			sc->nfingers = 2;
		break;
	case ALPS_V7_PACKETID_MULTIFINGER:
		sc->nfingers = 3 + (psc->packet[5] & 0x03);
		break;
	}

	button = (psc->packet[0] & 0x80) >> 7;
	buttons = 0;
	if (sc->nfingers == 1) {
		if (button && (cur_y1 > 1700) && (cur_x1 < 1700))
			buttons |= button << 0; /* Left button */
		else if (button && (cur_y1 > 1700)
				&& (1700 <= cur_x1) && (cur_x1 <= 2700))
			buttons |= button << 1; /* Middle button */
		else if (button && (cur_y1 > 1700) && (2700 < cur_x1))
			buttons |= button << 2; /* Right button */
	} else if (sc->nfingers > 1) {
		if (button && (cur_y2 > 1700) && (cur_x2 < 1700))
			buttons |= button << 0; /* Left button */
		else if (button && (cur_y2 > 1700)
				&& (1700 <= cur_x2) && (cur_x2 <= 2700))
			buttons |= button << 1; /* Middle button */
		else if (button && (cur_y2 > 1700) && (2700 < cur_x2))
			buttons |= button << 2; /* Right button */
	}

	/* New touch */
	if (sc->nfingers == 0 || sc->nfingers != sc->last_nfingers)
		sc->initializing = true;

	if (sc->initializing == true) {
		dx1 = 0;
		dy1 = 0;
	} else {
		dx1 = (int16_t)(cur_x1 - sc->last_x1);
		dy1 = (int16_t)(sc->last_y1 - cur_y1);

		dx1 = dx1 >> alps_touchpad_xy_unprecision;
		dy1 = dy1 >> alps_touchpad_xy_unprecision;
	}

	/* Allow finger detouch during drag and drop */
	if ((sc->nfingers < sc->last_nfingers)
		&& (cur_x2 == sc->last_x1) && (cur_y2 == sc->last_y1)) {
		sc->last_x1 = sc->last_x2;
		sc->last_y1 = sc->last_y2;
		dx1 = 0;
		dy1 = 0;
	}

	s = spltty();
	wsmouse_input(psc->sc_wsmousedev,
		buttons,
		dx1, dy1, 0, 0,
		WSMOUSE_INPUT_DELTA);
	splx(s);

	if (sc->initializing == true || (dx1 != 0))
		sc->last_x1 = cur_x1;
	if (sc->initializing == true || (dy1 != 0))
		sc->last_y1 = cur_y1;

	if (sc->nfingers > 0)
		sc->initializing = false;
	sc->last_nfingers = sc->nfingers;
}

static void
pms_alps_dispatch_packet_v7(struct pms_softc *psc)
{
	if ((psc->packet[0] == 0x48) && ((psc->packet[4] & 0x47) == 0x06))
		pms_alps_decode_trackstick_packet_v7(psc);
	else
		pms_alps_decode_touchpad_packet_v7(psc);
}

static void
pms_alps_decode_touchpad_packet_v2(struct pms_softc *psc)
{
	int s;
	struct alps_softc *sc = &psc->u.alps;
	uint16_t cur_x, cur_y;
	int16_t dx, dy;
	u_int left, middle, right;
	u_int forward, back;
	u_int buttons;
	uint8_t ges;

	sc->nfingers = (psc->packet[2] & 0x02) >> 1;
	if (sc->last_nfingers == 0)
		sc->initializing = true;

	left = psc->packet[3] & 0x01;
	right = (psc->packet[3] & 0x02) >> 1;
	middle = (psc->packet[3] & 0x04) >> 2;

	cur_x = psc->packet[1];
	cur_x |= (psc->packet[2] & 0x78) << 4;

	cur_y = psc->packet[4];
	cur_y |= (psc->packet[3] & 0x70) << 3;

#if 0
	cur_z = psc->packet[5];
#endif

	forward = (psc->packet[2] & 0x04) >> 2;
	back = (psc->packet[3] & 0x04) >> 2;
	ges = psc->packet[2] & 0x01;

	buttons = (left | ges) << 0;
	buttons |= (middle | forward | back) << 1;
	buttons |= right << 2;

	if (sc->initializing == true) {
		dx = 0;
		dy = 0;
	} else {
		dx = (cur_x - sc->last_x1);
		dy = (sc->last_y1 - cur_y);

		dx = dx >> alps_touchpad_xy_unprecision;
		dy = dy >> alps_touchpad_xy_unprecision;
	}

	s = spltty();
	wsmouse_input(psc->sc_wsmousedev,
		buttons,
		dx, dy, 0, 0,
		WSMOUSE_INPUT_DELTA);
	splx(s);

	if (sc->initializing == true || (dx != 0))
		sc->last_x1 = cur_x;
	if (sc->initializing == true || (dy != 0))
		sc->last_y1 = cur_y;

	if (sc->nfingers > 0)
		sc->initializing = false;
	sc->last_nfingers = sc->nfingers;
}

static void
pms_alps_input_v2(void *opaque, int data)
{
	struct pms_softc *psc = opaque;

	if (!psc->sc_enabled)
		return;

	psc->packet[psc->inputstate++] = data & 0xff;
	if (psc->inputstate < 6)
		return;

	pms_alps_decode_touchpad_packet_v2(psc);

	psc->inputstate = 0;
}

static void
pms_alps_input_v7(void *opaque, int data)
{
	struct pms_softc *psc = opaque;

	if (!psc->sc_enabled)
		return;

	psc->packet[psc->inputstate++] = data & 0xff;
	if (psc->inputstate < 6)
		return;

	pms_alps_dispatch_packet_v7(psc);

	psc->inputstate = 0;
}

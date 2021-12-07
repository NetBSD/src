/*	$NetBSD: common.c,v 1.1 2021/12/07 17:39:55 brad Exp $	*/

/*
 * Copyright (c) 2021 Brad Spencer <brad@anduin.eldar.org>
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

#ifdef __RCSID
__RCSID("$NetBSD: common.c,v 1.1 2021/12/07 17:39:55 brad Exp $");
#endif

/* Common functions dealing with the SCMD devices.  This does not
 * know how to talk to anything in particular, it calls out to the
 * functions defined in the function blocks for that.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include <dev/ic/scmdreg.h>

#define EXTERN
#include "common.h"
#include "responses.h"
#include "scmdctl.h"


int
decode_motor_level(int raw)
{
	int r;

	r = abs(128 - raw);
	if (raw < 128)
		r = r * -1;

	return r;
}

int common_clear(struct function_block *fb, int fd, bool debug)
{
	return (*(fb->func_clear))(fd, debug);
}

int
common_identify(struct function_block *fb, int fd, bool debug, int a_module, struct scmd_identify_response *r)
{
	uint8_t b;
	int err;

	err = (*(fb->func_clear))(fd, debug);
	if (! err) {
		err = (*(fb->func_phy_read))(fd, debug, a_module, SCMD_REG_ID, SCMD_REG_ID, &b);
		if (! err)
			r->id = b;
		err = (*(fb->func_phy_read))(fd, debug, a_module, SCMD_REG_FID, SCMD_REG_FID, &b);
		if (! err)
			r->fwversion = b;
		err = (*(fb->func_phy_read))(fd, debug, a_module, SCMD_REG_CONFIG_BITS, SCMD_REG_CONFIG_BITS, &b);
		if (! err)
			r->config_bits = b;
		err = (*(fb->func_phy_read))(fd, debug, a_module, SCMD_REG_SLAVE_ADDR, SCMD_REG_SLAVE_ADDR, &b);
		if (! err)
			r->slv_i2c_address = b;
	}

	return err;
}

int common_diag(struct function_block *fb, int fd, bool debug, int a_module, struct scmd_diag_response *r)
{
	uint8_t b;
	int err, m;

	err = (*(fb->func_clear))(fd, debug);
	if (! err) {
		m = 0;
		for(uint8_t n = SCMD_REG_U_I2C_RD_ERR; n <= SCMD_REG_GEN_TEST_WORD; n++) {
			err = (*(fb->func_phy_read))(fd, debug, a_module, n, n, &b);
			if (! err) {
				r->diags[m] = b;
			}
			m++;
		}
	}

	return err;
}

/* This tries to avoid reading just every register if only one
 * motor is asked about.
 */
int
common_get_motor(struct function_block *fb, int fd, bool debug, int a_module, struct scmd_motor_response *r)
{
	uint8_t b;
	int err = 0,m;

	if (a_module != SCMD_ANY_MODULE &&
	    (a_module < 0 || a_module > 16))
		return EINVAL;

	err = (*(fb->func_clear))(fd, debug);
	if (! err) {
		err = (*(fb->func_phy_read))(fd, debug, 0, SCMD_REG_DRIVER_ENABLE, SCMD_REG_DRIVER_ENABLE, &b);
		if (! err)
			r->driver = b;

		m = 0;
		for(uint8_t n = SCMD_REG_MA_DRIVE; n <= SCMD_REG_S16B_DRIVE; n++) {
			r->motorlevels[m] = SCMD_NO_MOTOR;
			if (a_module != SCMD_ANY_MODULE &&
			    (m / 2) != a_module)
				goto skip;
			err = (*(fb->func_phy_read))(fd, debug, 0, n, n, &b);
			if (! err)
				r->motorlevels[m] = b;
 skip:
			m++;
		}

		if (a_module == SCMD_ANY_MODULE ||
		    a_module == 0) {
			err = (*(fb->func_phy_read))(fd, debug, 0, SCMD_REG_MOTOR_A_INVERT, SCMD_REG_MOTOR_A_INVERT, &b);
			if (!err)
				r->invert[0] = (b & 0x01);
			err = (*(fb->func_phy_read))(fd, debug, 0, SCMD_REG_MOTOR_B_INVERT, SCMD_REG_MOTOR_B_INVERT, &b);
			if (!err)
				r->invert[1] = (b & 0x01);
		}

		if (a_module != 0) {
			m = 2;
			for(uint8_t n = SCMD_REG_INV_2_9; n <= SCMD_REG_INV_26_33; n++) {
				err = (*(fb->func_phy_read))(fd, debug, 0, n, n, &b);
				if (!err) {
					for(uint8_t j = 0; j < 8;j++) {
						r->invert[m] = (b & (1 << j));
						m++;
					}
				}
			}
		}

		if (a_module == SCMD_ANY_MODULE ||
		    a_module == 0) {
			err = (*(fb->func_phy_read))(fd, debug, 0, SCMD_REG_BRIDGE, SCMD_REG_BRIDGE, &b);
			if (!err)
				r->bridge[0] = (b & 0x01);
		}

		if (a_module != 0) {
			m = 1;
			for(uint8_t n = SCMD_REG_BRIDGE_SLV_L; n <= SCMD_REG_BRIDGE_SLV_H; n++) {
				err = (*(fb->func_phy_read))(fd, debug, 0, n, n, &b);
				if (!err) {
					for(uint8_t j = 0; j < 8;j++) {
						r->bridge[m] = (b & (1 << j));
						m++;
					}
				}
			}
		}
	}

	return err;
}

int
common_set_motor(struct function_block *fb, int fd, bool debug, int a_module, char a_motor, int8_t reg_v)
{
	uint8_t reg;
	int err;
	int reg_index;

	if (a_module < 0 || a_module > 16)
		return EINVAL;

	if (!(a_motor == 'A' || a_motor == 'B'))
		return EINVAL;

	err = (*(fb->func_clear))(fd, debug);
	if (! err) {
		reg_index = a_module * 2;
		if (a_motor == 'B')
			reg_index++;
		reg = SCMD_REG_MA_DRIVE + reg_index;
		reg_v = reg_v + 128;
		if (debug)
			fprintf(stderr,"common_set_motor: reg_index: %d ; reg: %02X ; reg_v: %d\n",reg_index,reg,reg_v);
		err = (*(fb->func_phy_write))(fd, debug, 0, reg, reg_v);
	}

	return err;
}

int
common_invert_motor(struct function_block *fb, int fd, bool debug, int a_module, char a_motor)
{
	uint8_t b;
	int err;
	uint8_t reg, reg_index = 0, reg_offset = 0;
	int motor_index;

	err = (*(fb->func_clear))(fd, debug);
	if (! err) {
		if (a_module == 0) {
			if (a_motor == 'A') {
				reg = SCMD_REG_MOTOR_A_INVERT;
			} else {
				reg = SCMD_REG_MOTOR_B_INVERT;
			}
			err = (*(fb->func_phy_read))(fd, debug, 0, reg, reg, &b);
			if (!err) {
				b = b ^ 0x01;
				err = (*(fb->func_phy_write))(fd, debug, 0, reg, b);
			}
		} else {
			motor_index = (a_module * 2) - 2;
			if (a_motor == 'B')
				motor_index++;
			reg_offset = motor_index / 8;
			motor_index = motor_index % 8;
			reg_index = 1 << motor_index;
			reg = SCMD_REG_INV_2_9 + reg_offset;
			if (debug)
				fprintf(stderr,"common_invert_motor: remote invert: motor_index: %d ; reg_offset: %d ; reg_index: %02X ; reg: %02X\n",motor_index,reg_offset,reg_index,reg);
			err = (*(fb->func_phy_read))(fd, debug, 0, reg, reg, &b);
			if (!err) {
				b = b ^ reg_index;
				err = (*(fb->func_phy_write))(fd, debug, 0, reg, b);
			}
		}
	}

	return err;
}

int
common_bridge_motor(struct function_block *fb, int fd, bool debug, int a_module)
{
	uint8_t b;
	int err = 0;
	uint8_t reg, reg_index = 0, reg_offset = 0;
	int module_index;

	err = (*(fb->func_clear))(fd, debug);
	if (! err) {
		if (a_module == 0) {
			err = (*(fb->func_phy_read))(fd, debug, 0, SCMD_REG_BRIDGE, SCMD_REG_BRIDGE, &b);
			if (!err) {
				b = b ^ 0x01;
				err = (*(fb->func_phy_write))(fd, debug, 0, SCMD_REG_BRIDGE, b);
			}
		} else {
			module_index = a_module - 1;
			reg_offset = module_index / 8;
			module_index = module_index % 8;
			reg_index = 1 << module_index;
			reg = SCMD_REG_BRIDGE_SLV_L + reg_offset;
			if (debug)
				fprintf(stderr,"common_bridge_motor: remote bridge: module_index: %d ; reg_offset: %d ; reg_index: %02X ; reg: %02X\n",module_index,reg_offset,reg_index,reg);
			err = (*(fb->func_phy_read))(fd, debug, 0, reg, reg, &b);
			if (!err) {
				b = b ^ reg_index;
				err = (*(fb->func_phy_write))(fd, debug, 0, reg, b);
			}
		}
	}

	return err;
}

int
common_enable_disable(struct function_block *fb, int fd, bool debug, int subcmd)
{
	int err;
	uint8_t reg_v;

	if (!(subcmd == SCMD_ENABLE || subcmd == SCMD_DISABLE))
		return EINVAL;

	err = (*(fb->func_clear))(fd, debug);
	if (! err) {
		switch (subcmd) {
		case SCMD_ENABLE:
			reg_v = SCMD_DRIVER_ENABLE;
			break;
		case SCMD_DISABLE:
			reg_v = SCMD_DRIVER_DISABLE;
			break;
		default:
			return EINVAL;
		}
		err = (*(fb->func_phy_write))(fd, debug, 0, SCMD_REG_DRIVER_ENABLE, reg_v);
	}

	return err;
}

/* These control commands can take a very long time and the restart
 * make cause the device to become unresponsive for a bit.
 */
int
common_control_1(struct function_block *fb, int fd, bool debug, int subcmd)
{
	int err;
	uint8_t reg_v;

	if (!(subcmd == SCMD_RESTART || subcmd == SCMD_ENUMERATE))
		return EINVAL;

	err = (*(fb->func_clear))(fd, debug);
	if (! err) {
		switch (subcmd) {
		case SCMD_RESTART:
			reg_v = SCMD_CONTROL_1_RESTART;
			break;
		case SCMD_ENUMERATE:
			reg_v = SCMD_CONTROL_1_REENUMERATE;
			break;
		default:
			return EINVAL;
		}
		err = (*(fb->func_phy_write))(fd, debug, 0, SCMD_REG_CONTROL_1, reg_v);
	}

	return err;
}

int
common_get_update_rate(struct function_block *fb, int fd, bool debug, uint8_t *rate)
{
	uint8_t b;
	int err;

	err = (*(fb->func_clear))(fd, debug);
	if (! err) {
		err = (*(fb->func_phy_read))(fd, debug, 0, SCMD_REG_UPDATE_RATE, SCMD_REG_UPDATE_RATE, &b);
		if (!err)
			*rate = b;
	}

	return err;
}

int
common_set_update_rate(struct function_block *fb, int fd, bool debug, uint8_t rate)
{
	int err;

	err = (*(fb->func_clear))(fd, debug);
	if (! err) {
		err = (*(fb->func_phy_write))(fd, debug, 0, SCMD_REG_UPDATE_RATE, rate);
	}

	return err;
}

int
common_force_update(struct function_block *fb, int fd, bool debug)
{
	int err;

	err = (*(fb->func_clear))(fd, debug);
	if (! err) {
		err = (*(fb->func_phy_write))(fd, debug, 0, SCMD_REG_FORCE_UPDATE, 0x01);
	}

	return err;
}

int
common_get_ebus_speed(struct function_block *fb, int fd, bool debug, uint8_t *speed)
{
	uint8_t b;
	int err;

	err = (*(fb->func_clear))(fd, debug);
	if (! err) {
		err = (*(fb->func_phy_read))(fd, debug, 0, SCMD_REG_E_BUS_SPEED, SCMD_REG_E_BUS_SPEED, &b);
		if (!err)
			*speed = b;
	}

	return err;
}

int
common_set_ebus_speed(struct function_block *fb, int fd, bool debug, uint8_t speed)
{
	int err;

	if (speed > 0x03)
		return EINVAL;

	err = (*(fb->func_clear))(fd, debug);
	if (! err) {
		err = (*(fb->func_phy_write))(fd, debug, 0, SCMD_REG_E_BUS_SPEED, speed);
	}

	return err;
}

int
common_get_lock_state(struct function_block *fb, int fd, bool debug, int ltype, uint8_t *lstate)
{
	uint8_t b;
	uint8_t reg;
	int err;

	switch (ltype) {
	case SCMD_LOCAL_USER_LOCK:
		reg = SCMD_REG_LOCAL_USER_LOCK;
		break;
	case SCMD_LOCAL_MASTER_LOCK:
		reg = SCMD_REG_LOCAL_MASTER_LOCK;
		break;
	case SCMD_GLOBAL_USER_LOCK:
		reg = SCMD_REG_USER_LOCK;
		break;
	case SCMD_GLOBAL_MASTER_LOCK:
		reg = SCMD_REG_MASTER_LOCK;
		break;
	default:
		return EINVAL;
	}

	err = (*(fb->func_clear))(fd, debug);
	if (! err) {
		err = (*(fb->func_phy_read))(fd, debug, 0, reg, reg, &b);
		if (!err)
			*lstate = b;
	}

	return err;
}

int
common_set_lock_state(struct function_block *fb, int fd, bool debug, int ltype, uint8_t lstate)
{
	uint8_t reg;
	uint8_t state;
	int err;

	switch (ltype) {
	case SCMD_LOCAL_USER_LOCK:
		reg = SCMD_REG_LOCAL_USER_LOCK;
		break;
	case SCMD_LOCAL_MASTER_LOCK:
		reg = SCMD_REG_LOCAL_MASTER_LOCK;
		break;
	case SCMD_GLOBAL_USER_LOCK:
		reg = SCMD_REG_USER_LOCK;
		break;
	case SCMD_GLOBAL_MASTER_LOCK:
		reg = SCMD_REG_MASTER_LOCK;
		break;
	default:
		return EINVAL;
	}

	switch (lstate) {
	case SCMD_LOCK_LOCKED:
		state = SCMD_ANY_LOCK_LOCKED;
		break;
	case SCMD_LOCK_UNLOCK:
		state = SCMD_MASTER_LOCK_UNLOCKED;
		if (ltype == SCMD_LOCAL_USER_LOCK ||
		    ltype == SCMD_GLOBAL_USER_LOCK)
			state = SCMD_USER_LOCK_UNLOCKED;
		break;
	default:
		return EINVAL;
	}

	err = (*(fb->func_clear))(fd, debug);
	if (! err) {
		err = (*(fb->func_phy_write))(fd, debug, 0, reg, state);
	}

	return err;
}

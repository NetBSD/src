/*	$NetBSD: umcpmio_gpio.c,v 1.1 2025/11/29 18:39:14 brad Exp $	*/

/*
 * Copyright (c) 2024, 2025 Brad Spencer <brad@anduin.eldar.org>
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
__KERNEL_RCSID(0, "$NetBSD: umcpmio_gpio.c,v 1.1 2025/11/29 18:39:14 brad Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/types.h>

#include <sys/gpio.h>

#include <dev/gpio/gpiovar.h>

#include <dev/usb/umcpmio.h>
#include <dev/usb/umcpmio_hid_reports.h>
#include <dev/usb/umcpmio_transport.h>
#include <dev/usb/umcpmio_gpio.h>
#include <dev/usb/umcpmio_subr.h>

#define UMCPMIO_DEBUG 1
#ifdef UMCPMIO_DEBUG
#define DPRINTF(x)	do { if (umcpmiodebug) printf x; } while (0)
#define DPRINTFN(n, x)	do { if (umcpmiodebug > (n)) printf x; } while (0)
extern int umcpmiodebug;
#else
#define DPRINTF(x)	__nothing
#define DPRINTFN(n, x)	__nothing
#endif

/* Stuff required to deal with the gpio on the MCP2210 and
 * MCP2221 / MCP2221A */

/* The MCP2210 has more, but simpler, pins than the MCP2221
 * / MCP2221A.
 *
 *
 * The MCP2221 / MCP2221A does not have symetric behavior with
 * respect to the get and puts of the SRAM.  This means that you
 * more or less can't just take the response buffer from a SRAM
 * get and use it directly as a SRAM put.  The MCP2210 is better
 * in this respect.
 */


/* We call the dedicated function ALT3 everywhere */

static uint32_t
mcp2210_counter_gp_to_flags(uint8_t other_settings)
{
	uint32_t r = 0;

	switch ((other_settings >> 1) & 0x07) {
	case MCP2210_COUNTER_FALLING_EDGE:
		r |= GPIO_PIN_ALT3;
		break;
	case MCP2210_COUNTER_RISING_EDGE:
		r |= GPIO_PIN_ALT4;
		break;
	case MCP2210_COUNTER_LOW_PULSE:
		r |= GPIO_PIN_ALT5;
		break;
	case MCP2210_COUNTER_HIGH_PULSE:
		r |= GPIO_PIN_ALT6;
		break;
	case MCP2210_COUNTER_OFF:
	default:
		printf("mcp2210_counter_gp_to_flags: Unhandled flag on counter pin: 0x%02x\n", other_settings);
	}

	return r;
}

static uint32_t
mcp2210_sram_gpio_to_flags(uint8_t *gp_settings, int pin)
{
	uint32_t r = 0;

	switch (gp_settings[pin]) {
	case MCP2210_PIN_IS_ALT0:
		r |= GPIO_PIN_ALT0;
		break;
	case MCP2210_PIN_IS_DED:
		if (pin == 6) {
			r |= mcp2210_counter_gp_to_flags(gp_settings[11]);
		} else {
			r |= GPIO_PIN_ALT3;
		}
		break;
	case MCP2210_PIN_IS_GPIO:
	default:
		if (pin < 8) {
			if (gp_settings[9] & (1 << pin))
				r |= GPIO_PIN_INPUT;
			else
				r |= GPIO_PIN_OUTPUT;
		} else {
			r |= GPIO_PIN_INPUT;
		}
		break;
	}

	return r;
}

static uint32_t
mcp2221_sram_gpio_to_flags(uint8_t gp_setting)
{
	uint32_t r = 0;

	switch (gp_setting & MCP2221_SRAM_PIN_TYPE_MASK) {
	case MCP2221_SRAM_PIN_IS_DED:
		r |= GPIO_PIN_ALT3;
		break;
	case MCP2221_SRAM_PIN_IS_ALT0:
		r |= GPIO_PIN_ALT0;
		break;
	case MCP2221_SRAM_PIN_IS_ALT1:
		r |= GPIO_PIN_ALT1;
		break;
	case MCP2221_SRAM_PIN_IS_ALT2:
		r |= GPIO_PIN_ALT2;
		break;
	case MCP2221_SRAM_PIN_IS_GPIO:
	default:
		if ((gp_setting & MCP2221_SRAM_GPIO_TYPE_MASK) ==
		    MCP2221_SRAM_GPIO_INPUT)
			r |= GPIO_PIN_INPUT;
		else
			r |= GPIO_PIN_OUTPUT;
		break;
	}

	return r;
}

static void
mcp2221_set_gpio_dir_sram(struct mcp2221_set_sram_req *req, int pin, int flags)
{
	uint8_t *alter = NULL;
	uint8_t *newvalue = NULL;

	if (pin >= 0 && pin < MCP2221_NPINS) {
		switch (pin) {
		case 0:
			alter = &req->alter_gpio_config;
			newvalue = &req->gp0_settings;
			break;
		case 1:
			alter = &req->alter_gpio_config;
			newvalue = &req->gp1_settings;
			break;
		case 2:
			alter = &req->alter_gpio_config;
			newvalue = &req->gp2_settings;
			break;
		case 3:
			alter = &req->alter_gpio_config;
			newvalue = &req->gp3_settings;
			break;
		default:
			break;
		}

		if (alter != NULL) {
			*alter = MCP2221_SRAM_ALTER_GPIO;
			if (flags & GPIO_PIN_INPUT)
				*newvalue |= MCP2221_SRAM_GPIO_INPUT;
			else
				*newvalue &= ~MCP2221_SRAM_GPIO_INPUT;
		}
	}
}

static int
mcp2210_get_gpio_sram(struct umcpmio_softc *sc,
    struct mcp2210_get_gpio_sram_res *res)
{
	struct mcp2210_get_gpio_sram_req req;
	int err = 0;

	memset(&req, 0, MCP2210_REQ_BUFFER_SIZE);
	req.cmd = MCP2210_CMD_GET_GPIO_SRAM;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	err = umcpmio_send_report(sc,
	    (uint8_t *) & req, MCP2210_REQ_BUFFER_SIZE,
	    (uint8_t *) res, sc->sc_cv_wait);

	err = mcp2210_decode_errors(req.cmd, err, res->completion);

	return err;
}

static int
mcp2210_set_gpio_sram(struct umcpmio_softc *sc,
    struct mcp2210_set_gpio_sram_req *req, struct mcp2210_set_gpio_sram_res *res)
{
	int err = 0;

	req->cmd = MCP2210_CMD_SET_GPIO_SRAM;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	err = umcpmio_send_report(sc,
	    (uint8_t *) req, MCP2210_REQ_BUFFER_SIZE,
	    (uint8_t *) res, sc->sc_cv_wait);

	err = mcp2210_decode_errors(req->cmd, err, res->completion);

	return err;
}

static int
mcp2210_get_gpio_dir_sram(struct umcpmio_softc *sc,
    struct mcp2210_get_gpio_dir_res *res)
{
	struct mcp2210_get_gpio_dir_req req;
	int err = 0;

	memset(&req, 0, MCP2210_REQ_BUFFER_SIZE);
	req.cmd = MCP2210_CMD_GET_GPIO_DIR_SRAM;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	err = umcpmio_send_report(sc,
	    (uint8_t *) & req, MCP2210_REQ_BUFFER_SIZE,
	    (uint8_t *) res, sc->sc_cv_wait);

	err = mcp2210_decode_errors(req.cmd, err, res->completion);

	return err;
}

static int
mcp2210_get_gpio_value_sram(struct umcpmio_softc *sc,
    struct mcp2210_get_gpio_value_res *res)
{
	struct mcp2210_get_gpio_value_req req;
	int err = 0;

	memset(&req, 0, MCP2210_REQ_BUFFER_SIZE);
	req.cmd = MCP2210_CMD_GET_GPIO_VAL_SRAM;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	err = umcpmio_send_report(sc,
	    (uint8_t *) & req, MCP2210_REQ_BUFFER_SIZE,
	    (uint8_t *) res, sc->sc_cv_wait);

	err = mcp2210_decode_errors(req.cmd, err, res->completion);

	return err;
}

static int
mcp2210_set_gpio_dir_sram(struct umcpmio_softc *sc,
    struct mcp2210_set_gpio_dir_req *req, struct mcp2210_set_gpio_dir_res *res)
{
	int err = 0;

	req->cmd = MCP2210_CMD_SET_GPIO_DIR_SRAM;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	err = umcpmio_send_report(sc,
	    (uint8_t *) req, MCP2210_REQ_BUFFER_SIZE,
	    (uint8_t *) res, sc->sc_cv_wait);

	err = mcp2210_decode_errors(req->cmd, err, res->completion);

	return err;
}

static int
mcp2210_set_gpio_value_sram(struct umcpmio_softc *sc,
    struct mcp2210_set_gpio_value_req *req, struct mcp2210_set_gpio_value_res *res)
{
	int err = 0;

	req->cmd = MCP2210_CMD_SET_GPIO_VAL_SRAM;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	err = umcpmio_send_report(sc,
	    (uint8_t *) req, MCP2210_REQ_BUFFER_SIZE,
	    (uint8_t *) res, sc->sc_cv_wait);

	err = mcp2210_decode_errors(req->cmd, err, res->completion);

	return err;
}

static void
mcp2221_set_gpio_designation_sram(struct mcp2221_set_sram_req *req, int pin,
    int flags)
{
	uint8_t *alter = NULL;
	uint8_t *newvalue = NULL;
	uint32_t altmask =
	GPIO_PIN_ALT0 | GPIO_PIN_ALT1 | GPIO_PIN_ALT2 | GPIO_PIN_ALT3;

	if (pin >= 0 && pin < MCP2221_NPINS) {
		switch (pin) {
		case 0:
			alter = &req->alter_gpio_config;
			newvalue = &req->gp0_settings;
			break;
		case 1:
			alter = &req->alter_gpio_config;
			newvalue = &req->gp1_settings;
			break;
		case 2:
			alter = &req->alter_gpio_config;
			newvalue = &req->gp2_settings;
			break;
		case 3:
			alter = &req->alter_gpio_config;
			newvalue = &req->gp3_settings;
			break;
		default:
			break;
		}

		if (alter != NULL) {
			int nv = *newvalue;

			*alter = MCP2221_SRAM_ALTER_GPIO;
			nv &= 0xF8;

			if (flags & (GPIO_PIN_OUTPUT | GPIO_PIN_INPUT)) {
				nv |= MCP2221_SRAM_PIN_IS_GPIO;
			} else {
				switch (flags & altmask) {
				case GPIO_PIN_ALT0:
					nv |= MCP2221_SRAM_PIN_IS_ALT0;
					break;
				case GPIO_PIN_ALT1:
					nv |= MCP2221_SRAM_PIN_IS_ALT1;
					break;
				case GPIO_PIN_ALT2:
					nv |= MCP2221_SRAM_PIN_IS_ALT2;
					break;
					/* ALT3 will always be used as the
					 * dedicated function specific to the
					 * pin.  Not all of the pins will have
					 * the alt functions below #3. */
				case GPIO_PIN_ALT3:
					nv |= MCP2221_SRAM_PIN_IS_DED;
					break;
				default:
					break;
				}
			}
			*newvalue = nv;
		}
	}
}
/*
 * It is unfortunate that the GET and PUT requests are not symertric.  That is,
 * the bits sort of line up but not quite between a GET and PUT.
 */

static struct umcpmio_mapping_put mcp2221_vref_puts[] = {
	{
		.tname = "4.096V",
		.mask = 0x06 | 0x01,
	},
	{
		.tname = "2.048V",
		.mask = 0x04 | 0x01,
	},
	{
		.tname = "1.024V",
		.mask = 0x02 | 0x01,
	},
	{
		.tname = "OFF",
		.mask = 0x00 | 0x01,
	},
	{
		.tname = "VDD",
		.mask = 0x00,
	}
};

void
mcp2221_set_dac_vref(struct mcp2221_set_sram_req *req, char *newvref)
{
	int i;

	for (i = 0; i < __arraycount(mcp2221_vref_puts); i++) {
		if (strncmp(newvref, mcp2221_vref_puts[i].tname,
		    MCP2221_VREF_NAME) == 0) {
			break;
		}
	}

	if (i == __arraycount(mcp2221_vref_puts))
		return;

	req->dac_voltage_reference |= mcp2221_vref_puts[i].mask |
	    MCP2221_SRAM_CHANGE_DAC_VREF;
}

int
mcp2221_set_dac_vref_one(struct umcpmio_softc *sc, char *newvref)
{
	struct mcp2221_set_sram_req req;
	struct mcp2221_set_sram_res res;
	int err = 0;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	mcp2221_set_dac_vref(&req, newvref);
	err = mcp2221_put_sram(sc, &req, &res);

	return err;
}

void
mcp2221_set_dac_value(struct mcp2221_set_sram_req *req, uint8_t newvalue)
{
	req->set_dac_output_value |= (newvalue & MCP2221_SRAM_DAC_VALUE_MASK) |
	    MCP2221_SRAM_CHANGE_DAC_VREF;
}

int
mcp2221_set_dac_value_one(struct umcpmio_softc *sc, uint8_t newvalue)
{
	struct mcp2221_set_sram_req req;
	struct mcp2221_set_sram_res res;
	int err = 0;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	mcp2221_set_dac_value(&req, newvalue);
	err = mcp2221_put_sram(sc, &req, &res);

	return err;
}

void
mcp2221_set_adc_vref(struct mcp2221_set_sram_req *req, char *newvref)
{
	int i;

	for (i = 0; i < __arraycount(mcp2221_vref_puts); i++) {
		if (strncmp(newvref, mcp2221_vref_puts[i].tname,
		    MCP2221_VREF_NAME) == 0) {
			break;
		}
	}

	if (i == __arraycount(mcp2221_vref_puts))
		return;

	req->adc_voltage_reference |= mcp2221_vref_puts[i].mask |
	    MCP2221_SRAM_CHANGE_ADC_VREF;
}

int
mcp2221_set_adc_vref_one(struct umcpmio_softc *sc, char *newvref)
{
	struct mcp2221_set_sram_req req;
	struct mcp2221_set_sram_res res;
	int err = 0;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	mcp2221_set_adc_vref(&req, newvref);
	err = mcp2221_put_sram(sc, &req, &res);

	return err;
}

static struct umcpmio_mapping_put mcp2221_dc_puts[] = {
	{
		.tname = "75%",
		.mask = MCP2221_SRAM_GPIO_CLOCK_DC_75,
	},
	{
		.tname = "50%",
		.mask = MCP2221_SRAM_GPIO_CLOCK_DC_50,
	},
	{
		.tname = "25%",
		.mask = MCP2221_SRAM_GPIO_CLOCK_DC_25,
	},
	{
		.tname = "0%",
		.mask = MCP2221_SRAM_GPIO_CLOCK_DC_0,
	}
};

void
mcp2221_set_gpioclock_dc(struct mcp2221_set_sram_req *req, char *new_dc)
{
	int i;

	for (i = 0; i < __arraycount(mcp2221_dc_puts); i++) {
		if (strncmp(new_dc, mcp2221_dc_puts[i].tname,
		    MCP2221_DC_NAME) == 0) {
			break;
		}
	}

	if (i == __arraycount(mcp2221_dc_puts))
		return;

	req->clock_output_divider |= mcp2221_dc_puts[i].mask;
}

int
mcp2221_set_gpioclock_dc_one(struct umcpmio_softc *sc, char *new_dutycycle)
{
	struct mcp2221_get_sram_res current_sram_res;
	struct mcp2221_set_sram_req req;
	struct mcp2221_set_sram_res res;
	int err = 0;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	err = mcp2221_get_sram(sc, &current_sram_res);
	if (err)
		goto out;

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	mcp2221_set_gpioclock_dc(&req, new_dutycycle);
	DPRINTF(("mcp2221_set_gpioclock_dc_one:"
	    " req.clock_output_divider=%02x, current mask=%02x\n",
	    req.clock_output_divider,
	    (current_sram_res.clock_divider &
	    MCP2221_SRAM_GPIO_CLOCK_CD_MASK)));
	req.clock_output_divider |=
	    (current_sram_res.clock_divider &
	    MCP2221_SRAM_GPIO_CLOCK_CD_MASK) |
	    MCP2221_SRAM_GPIO_CHANGE_DCCD;
	DPRINTF(("mcp2221_set_gpioclock_dc_one:"
	    " SET req.clock_output_divider=%02x\n",
	    req.clock_output_divider));
	err = mcp2221_put_sram(sc, &req, &res);
out:
	return err;
}

static struct umcpmio_mapping_put mcp2221_cd_puts[] = {
	{
		.tname = "375kHz",
		.mask = MCP2221_SRAM_GPIO_CLOCK_CD_375KHZ,
	},
	{
		.tname = "750kHz",
		.mask = MCP2221_SRAM_GPIO_CLOCK_CD_750KHZ,
	},
	{
		.tname = "1.5MHz",
		.mask = MCP2221_SRAM_GPIO_CLOCK_CD_1P5MHZ,
	},
	{
		.tname = "3MHz",
		.mask = MCP2221_SRAM_GPIO_CLOCK_CD_3MHZ,
	},
	{
		.tname = "6MHz",
		.mask = MCP2221_SRAM_GPIO_CLOCK_CD_6MHZ,
	},
	{
		.tname = "12MHz",
		.mask = MCP2221_SRAM_GPIO_CLOCK_CD_12MHZ,
	},
	{
		.tname = "24MHz",
		.mask = MCP2221_SRAM_GPIO_CLOCK_CD_24MHZ,
	}
};

void
mcp2221_set_gpioclock_cd(struct mcp2221_set_sram_req *req, char *new_cd)
{
	int i;

	for (i = 0; i < __arraycount(mcp2221_cd_puts); i++) {
		if (strncmp(new_cd, mcp2221_cd_puts[i].tname,
		    MCP2221_CD_NAME) == 0) {
			break;
		}
	}

	if (i == __arraycount(mcp2221_cd_puts))
		return;

	req->clock_output_divider |= mcp2221_cd_puts[i].mask;
}

int
mcp2221_set_gpioclock_cd_one(struct umcpmio_softc *sc, char *new_clockdivider)
{
	struct mcp2221_get_sram_res current_sram_res;
	struct mcp2221_set_sram_req req;
	struct mcp2221_set_sram_res res;
	int err = 0;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	err = mcp2221_get_sram(sc, &current_sram_res);
	if (err)
		goto out;

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	mcp2221_set_gpioclock_cd(&req, new_clockdivider);
	DPRINTF(("mcp2221_set_gpioclock_cd_one:"
	    " req.clock_output_divider=%02x, current mask=%02x\n",
	    req.clock_output_divider,
	    (current_sram_res.clock_divider &
	    MCP2221_SRAM_GPIO_CLOCK_CD_MASK)));
	req.clock_output_divider |=
	    (current_sram_res.clock_divider &
	    MCP2221_SRAM_GPIO_CLOCK_DC_MASK) |
	    MCP2221_SRAM_GPIO_CHANGE_DCCD;
	DPRINTF(("mcp2221_set_gpioclock_cd_one:"
	    " SET req.clock_output_divider=%02x\n",
	    req.clock_output_divider));
	err = mcp2221_put_sram(sc, &req, &res);
out:
	return err;
}

int
mcp2221_get_gpio_cfg(struct umcpmio_softc *sc,
    struct mcp2221_get_gpio_cfg_res *res)
{
	struct mcp2221_get_gpio_cfg_req req;
	int err = 0;

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	req.cmd = MCP2221_CMD_GET_GPIO_CFG;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	err = umcpmio_send_report(sc,
	    (uint8_t *) & req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *) res, sc->sc_cv_wait);

	return err;
}

static int
mcp2221_put_gpio_cfg(struct umcpmio_softc *sc,
    struct mcp2221_set_gpio_cfg_req *req, struct mcp2221_set_gpio_cfg_res *res)
{
	int err = 0;

	req->cmd = MCP2221_CMD_SET_GPIO_CFG;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	err = umcpmio_send_report(sc,
	    (uint8_t *) req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *) res, sc->sc_cv_wait);

	return err;
}

static int
mcp2210_get_gpio_value(struct umcpmio_softc *sc,
    int pin)
{
	struct mcp2210_get_gpio_value_res res;
	uint16_t v;
	int err = 0;
	int r = GPIO_PIN_LOW;


	KASSERT(mutex_owned(&sc->sc_action_mutex));

	err = mcp2210_get_gpio_value_sram(sc, &res);

	if (!err) {
		v = (res.pin_value_msb << 8) | res.pin_value_lsb;
		if (v & (1 << pin))
			r = GPIO_PIN_HIGH;
	}
	return r;
}
/* So... if the pin isn't set to GPIO, just call the output LOW */

static int
mcp2221_get_gpio_value(struct umcpmio_softc *sc,
    int pin)
{
	struct mcp2221_get_gpio_cfg_res get_gpio_cfg_res;
	int err = 0;
	int r = GPIO_PIN_LOW;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	err = mcp2221_get_gpio_cfg(sc, &get_gpio_cfg_res);
	if (err)
		goto out;

	if (get_gpio_cfg_res.cmd != MCP2221_CMD_GET_GPIO_CFG ||
	    get_gpio_cfg_res.completion != MCP2221_CMD_COMPLETE_OK) {
		device_printf(sc->sc_dev, "mcp2221_get_gpio_value:"
		    " wrong command or error: %02x %02x\n",
		    get_gpio_cfg_res.cmd,
		    get_gpio_cfg_res.completion);
		goto out;
	}
	switch (pin) {
	case 0:
		if (get_gpio_cfg_res.gp0_pin_value !=
		    MCP2221_GPIO_CFG_VALUE_NOT_GPIO)
			if (get_gpio_cfg_res.gp0_pin_value == 0x01)
				r = GPIO_PIN_HIGH;
		break;
	case 1:
		if (get_gpio_cfg_res.gp1_pin_value !=
		    MCP2221_GPIO_CFG_VALUE_NOT_GPIO)
			if (get_gpio_cfg_res.gp1_pin_value == 0x01)
				r = GPIO_PIN_HIGH;
		break;
	case 2:
		if (get_gpio_cfg_res.gp2_pin_value !=
		    MCP2221_GPIO_CFG_VALUE_NOT_GPIO)
			if (get_gpio_cfg_res.gp2_pin_value == 0x01)
				r = GPIO_PIN_HIGH;
		break;
	case 3:
		if (get_gpio_cfg_res.gp3_pin_value !=
		    MCP2221_GPIO_CFG_VALUE_NOT_GPIO)
			if (get_gpio_cfg_res.gp3_pin_value == 0x01)
				r = GPIO_PIN_HIGH;
		break;
	default:
		break;
	}
out:
	return r;
}

static int
mcp2210_set_gpio_value(struct umcpmio_softc *sc,
    int pin, bool value)
{
	int err = 0;
	struct mcp2210_get_gpio_value_res get_res;
	struct mcp2210_set_gpio_value_req set_req;
	struct mcp2210_set_gpio_value_res set_res;
	uint16_t v;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	err = mcp2210_get_gpio_value_sram(sc, &get_res);
	if (!err && get_res.completion != MCP2210_CMD_COMPLETE_OK)
		err = EIO;

	if (!err) {
		v = (get_res.pin_value_msb << 8) | get_res.pin_value_lsb;

		if (value)
			v |= (1 << pin);
		else
			v &= ~(1 << pin);

		set_req.pin_value_lsb = v & 0x00ff;
		set_req.pin_value_msb = (v & 0xff00) >> 8;

		err = mcp2210_set_gpio_value_sram(sc, &set_req, &set_res);
	}
	return err;
}

static void
mcp2221_set_gpio_value(struct mcp2221_set_gpio_cfg_req *req,
    int pin, bool value)
{
	uint8_t *alter = NULL;
	uint8_t *newvalue = NULL;

	if (pin < 0 || pin >= MCP2221_NPINS)
		return;

	switch (pin) {
	case 0:
		alter = &req->alter_gp0_value;
		newvalue = &req->new_gp0_value;
		break;
	case 1:
		alter = &req->alter_gp1_value;
		newvalue = &req->new_gp1_value;
		break;
	case 2:
		alter = &req->alter_gp2_value;
		newvalue = &req->new_gp2_value;
		break;
	case 3:
		alter = &req->alter_gp3_value;
		newvalue = &req->new_gp3_value;
		break;
	default:
		return;
	}

	*alter = MCP2221_GPIO_CFG_ALTER;
	*newvalue = 0;
	if (value)
		*newvalue = 1;
}

static int
mcp2221_set_gpio_value_one(struct umcpmio_softc *sc,
    int pin, bool value)
{
	int err = 0;
	struct mcp2221_set_gpio_cfg_req req;
	struct mcp2221_set_gpio_cfg_res res;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	mcp2221_set_gpio_value(&req, pin, value);
	err = mcp2221_put_gpio_cfg(sc, &req, &res);
	if (err)
		goto out;
	if (res.cmd != MCP2221_CMD_SET_GPIO_CFG ||
	    res.completion != MCP2221_CMD_COMPLETE_OK) {
		err = EIO;
		device_printf(sc->sc_dev, "umcpmio_gpio_pin_write:"
		    "  not the command desired, or error: %02x %02x\n",
		    res.cmd,
		    res.completion);
	}
out:
	return err;
}

/* These are standard gpio reads and set calls */

static int
umcpmio_gpio_pin_read(void *arg, int pin)
{
	struct umcpmio_softc *sc = arg;
	int r = GPIO_PIN_LOW;

	mutex_enter(&sc->sc_action_mutex);

	if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2210)
		r = mcp2210_get_gpio_value(sc, pin);
	if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2221)
		r = mcp2221_get_gpio_value(sc, pin);

	mutex_exit(&sc->sc_action_mutex);

	return r;
}

static void
umcpmio_gpio_pin_write(void *arg, int pin, int value)
{
	struct umcpmio_softc *sc = arg;

	mutex_enter(&sc->sc_action_mutex);

	if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2210)
		mcp2210_set_gpio_value(sc, pin, value);
	if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2221)
		mcp2221_set_gpio_value_one(sc, pin, value);

	mutex_exit(&sc->sc_action_mutex);
}

static int
mcp2210_gpio_pin_ctl(void *arg, int pin, int flags)
{
	struct umcpmio_softc *sc = arg;
	struct mcp2210_set_gpio_sram_req mcp2210_set_gpio_sram_req;
	struct mcp2210_set_gpio_dir_req mcp2210_set_gpio_dir_sram_req;
	struct mcp2210_set_gpio_sram_res mcp2210_set_gpio_sram_res;
	struct mcp2210_set_gpio_dir_res mcp2210_set_gpio_dir_sram_res;
	struct mcp2210_set_gpio_value_req mcp2210_set_gpio_value_sram_req;
	struct mcp2210_set_gpio_value_res mcp2210_set_gpio_value_sram_res;

	struct mcp2210_get_gpio_sram_res mcp2210_get_gpio_sram_res;
	struct mcp2210_get_gpio_dir_res mcp2210_get_gpio_dir_sram_res;
	struct mcp2210_get_gpio_value_res mcp2210_get_gpio_value_sram_res;

	uint32_t altmask =
	GPIO_PIN_ALT0 | GPIO_PIN_ALT3 | GPIO_PIN_ALT4 | GPIO_PIN_ALT5 | GPIO_PIN_ALT6;
	int err = 0;
	uint16_t vdir;

	if (sc->sc_dying)
		return 0;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	err = mcp2210_get_gpio_sram(sc, &mcp2210_get_gpio_sram_res);
	if (err)
		goto out;
	err = mcp2210_get_gpio_dir_sram(sc, &mcp2210_get_gpio_dir_sram_res);
	if (err)
		goto out;
	err = mcp2210_get_gpio_value_sram(sc, &mcp2210_get_gpio_value_sram_res);
	if (err)
		goto out;

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *) & mcp2210_get_gpio_sram_res, MCP2210_RES_BUFFER_SIZE,
	    "mcp2210_gpio_pin_ctl get gpio sram res");
	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *) & mcp2210_get_gpio_dir_sram_res, MCP2210_RES_BUFFER_SIZE,
	    "mcp2210_gpio_pin_ctl get gpio dir sram res");

	memset(&mcp2210_set_gpio_sram_req.cmd, 0, MCP2210_REQ_BUFFER_SIZE);
	memcpy(&mcp2210_set_gpio_sram_req.gp0_designation,
	    &mcp2210_get_gpio_sram_res.gp0_designation,
	    &mcp2210_get_gpio_sram_res.nvram_protection -
	    &mcp2210_get_gpio_sram_res.gp0_designation + 1);

	memset(&mcp2210_set_gpio_dir_sram_req.cmd, 0, MCP2210_REQ_BUFFER_SIZE);
	vdir = (mcp2210_get_gpio_dir_sram_res.pin_dir_msb << 8) |
	    mcp2210_get_gpio_dir_sram_res.pin_dir_lsb;

	uint8_t *b = (uint8_t *) & mcp2210_set_gpio_sram_req.cmd;
	bool changed_sram = false;

	if (flags & (GPIO_PIN_OUTPUT | GPIO_PIN_INPUT)) {
		if (b[MCP2210_GPIO_SRAM_GP0 + pin] != MCP2210_PIN_IS_GPIO) {
			b[MCP2210_GPIO_SRAM_GP0 + pin] = MCP2210_PIN_IS_GPIO;
			changed_sram = true;
		}
		if (flags & GPIO_PIN_INPUT)
			vdir |= (1 << pin);
		else
			vdir &= ~(1 << pin);
	} else {
		switch (flags & altmask) {
		case GPIO_PIN_ALT0:
			if (b[MCP2210_GPIO_SRAM_GP0 + pin] != MCP2210_PIN_IS_ALT0) {
				b[MCP2210_GPIO_SRAM_GP0 + pin] = MCP2210_PIN_IS_ALT0;
				changed_sram = true;
			}
			break;
		case GPIO_PIN_ALT3:
			if (b[MCP2210_GPIO_SRAM_GP0 + pin] != MCP2210_PIN_IS_DED) {
				b[MCP2210_GPIO_SRAM_GP0 + pin] = MCP2210_PIN_IS_DED;
				changed_sram = true;
			}
			if (pin == 6) {
				if (mcp2210_counter_gp_to_flags(mcp2210_get_gpio_sram_res.other_settings) != GPIO_PIN_ALT3) {
					mcp2210_set_gpio_sram_req.other_settings &= 0xf1;
					mcp2210_set_gpio_sram_req.other_settings |= MCP2210_COUNTER_FALLING_EDGE << 1;
					changed_sram = true;
				}
			}
			break;
		case GPIO_PIN_ALT4:
			if (b[MCP2210_GPIO_SRAM_GP0 + pin] != MCP2210_PIN_IS_DED) {
				b[MCP2210_GPIO_SRAM_GP0 + pin] = MCP2210_PIN_IS_DED;
				changed_sram = true;
			}
			if (pin == 6) {
				if (mcp2210_counter_gp_to_flags(mcp2210_get_gpio_sram_res.other_settings) != GPIO_PIN_ALT4) {
					mcp2210_set_gpio_sram_req.other_settings &= 0xf1;
					mcp2210_set_gpio_sram_req.other_settings |= MCP2210_COUNTER_RISING_EDGE << 1;
					changed_sram = true;
				}
			}
			break;
		case GPIO_PIN_ALT5:
			if (b[MCP2210_GPIO_SRAM_GP0 + pin] != MCP2210_PIN_IS_DED) {
				b[MCP2210_GPIO_SRAM_GP0 + pin] = MCP2210_PIN_IS_DED;
				changed_sram = true;
			}
			if (pin == 6) {
				if (mcp2210_counter_gp_to_flags(mcp2210_get_gpio_sram_res.other_settings) != GPIO_PIN_ALT5) {
					mcp2210_set_gpio_sram_req.other_settings &= 0xf1;
					mcp2210_set_gpio_sram_req.other_settings |= MCP2210_COUNTER_LOW_PULSE << 1;
					changed_sram = true;
				}
			}
			break;
		case GPIO_PIN_ALT6:
			if (b[MCP2210_GPIO_SRAM_GP0 + pin] != MCP2210_PIN_IS_DED) {
				b[MCP2210_GPIO_SRAM_GP0 + pin] = MCP2210_PIN_IS_DED;
				changed_sram = true;
			}
			if (pin == 6) {
				if (mcp2210_counter_gp_to_flags(mcp2210_get_gpio_sram_res.other_settings) != GPIO_PIN_ALT6) {
					mcp2210_set_gpio_sram_req.other_settings &= 0xf1;
					mcp2210_set_gpio_sram_req.other_settings |= MCP2210_COUNTER_HIGH_PULSE << 1;
					changed_sram = true;
				}
			}
			break;
		default:
			break;
		}
	}

	/* On the MCP-2210, if you change the purpose of the pin you have to
	 * write the current direction and value of the pins back to the chip.
	 * The reason for this is tha you can not change just one pins purpose
	 * without setting the direction and values of all pins to the default. */

	if (changed_sram) {
		err = mcp2210_set_gpio_sram(sc, &mcp2210_set_gpio_sram_req,
		    &mcp2210_set_gpio_sram_res);
		if (err)
			goto out;

		mcp2210_set_gpio_dir_sram_req.pin_dir_msb = (vdir >> 8) & 0xff;
		mcp2210_set_gpio_dir_sram_req.pin_dir_lsb = vdir & 0x00ff;

		err = mcp2210_set_gpio_dir_sram(sc, &mcp2210_set_gpio_dir_sram_req,
		    &mcp2210_set_gpio_dir_sram_res);
		if (err)
			goto out;

		mcp2210_set_gpio_value_sram_req.pin_value_msb = mcp2210_get_gpio_value_sram_res.pin_value_msb;
		mcp2210_set_gpio_value_sram_req.pin_value_lsb = mcp2210_get_gpio_value_sram_res.pin_value_lsb;

		/* Further, if the pin is for OUTPUT, then we will want to set
		 * its value to the default, otherwise the default may never be
		 * reflected in the pin state, as the pin might have been a
		 * INPUT with the opposite value and just putting all of the
		 * values back in that case would do the wrong thing. */

		if (flags & GPIO_PIN_OUTPUT && pin < 8) {
			if (mcp2210_set_gpio_sram_req.default_output_lsb & (1 << pin))
				mcp2210_set_gpio_value_sram_req.pin_value_lsb |= (1 << pin);
			else
				mcp2210_set_gpio_value_sram_req.pin_value_lsb &= ~(1 << pin);
		}
		err = mcp2210_set_gpio_value_sram(sc, &mcp2210_set_gpio_value_sram_req, &mcp2210_set_gpio_value_sram_res);
	} else {
		/* In this case, the pin purpose was not changed, so all that
		 * needs to happen is the direction needs to be updated.  This
		 * actually won't matter unless the pin is strickly a GPIO pin.
		 * ALT0, the CS purpose, is handled by the chip itself, ALT3 -
		 * ALT6 is for the event / interrupt counter.  So, we really
		 * only have to care if the pin switches from INPUT to OUTPUT,
		 * or vise versa. */
		if (flags & (GPIO_PIN_OUTPUT | GPIO_PIN_INPUT)) {
			mcp2210_set_gpio_dir_sram_req.pin_dir_msb = (vdir >> 8) & 0xff;
			mcp2210_set_gpio_dir_sram_req.pin_dir_lsb = vdir & 0x00ff;

			err = mcp2210_set_gpio_dir_sram(sc, &mcp2210_set_gpio_dir_sram_req,
			    &mcp2210_set_gpio_dir_sram_res);
		}
	}
out:
	return err;
}
/*
 * Internal function that does the dirty work of setting a gpio
 * pin to its "type" for the MCP-2221 / MCP-2221A
 *
 * There are really two ways to do some of this, one is to set the pin
 * to input and output, or whatever, using SRAM calls, the other is to
 * use the GPIO config calls to set input and output and SRAM for
 * everything else.  This just uses SRAM for everything.
 */

int
mcp2221_gpio_pin_ctl(void *arg, int pin, int flags)
{
	struct umcpmio_softc *sc = arg;
	struct mcp2221_set_sram_req set_sram_req;
	struct mcp2221_set_sram_res set_sram_res;
	struct mcp2221_get_sram_res current_sram_res;
	struct mcp2221_get_gpio_cfg_res current_gpio_cfg_res;
	int err = 0;

	if (sc->sc_dying)
		return 0;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	err = mcp2221_get_sram(sc, &current_sram_res);
	if (err)
		goto out;

	err = mcp2221_get_gpio_cfg(sc, &current_gpio_cfg_res);
	if (err)
		goto out;

	/* You can't just set one pin, you must set all of them, so copy the
	 * current settings for the pin we are not messing with.
	 *
	 * And, yes, of course, if the MCP-2210 is ever supported with this
	 * driver, this sort of unrolling will need to be turned into something
	 * different, but for now, just unroll as there are only 4 pins to care
	 * about.
	 *
	 * */

	memset(&set_sram_req, 0, MCP2221_REQ_BUFFER_SIZE);
	switch (pin) {
	case 0:
		set_sram_req.gp1_settings = current_sram_res.gp1_settings;
		set_sram_req.gp2_settings = current_sram_res.gp2_settings;
		set_sram_req.gp3_settings = current_sram_res.gp3_settings;
		break;
	case 1:
		set_sram_req.gp0_settings = current_sram_res.gp0_settings;
		set_sram_req.gp2_settings = current_sram_res.gp2_settings;
		set_sram_req.gp3_settings = current_sram_res.gp3_settings;
		break;
	case 2:
		set_sram_req.gp0_settings = current_sram_res.gp0_settings;
		set_sram_req.gp1_settings = current_sram_res.gp1_settings;
		set_sram_req.gp3_settings = current_sram_res.gp3_settings;
		break;
	case 3:
		set_sram_req.gp0_settings = current_sram_res.gp0_settings;
		set_sram_req.gp1_settings = current_sram_res.gp1_settings;
		set_sram_req.gp2_settings = current_sram_res.gp2_settings;
		break;
	}
	mcp2221_set_gpio_designation_sram(&set_sram_req, pin, flags);
	mcp2221_set_gpio_dir_sram(&set_sram_req, pin, flags);

	/* This part is unfortunate...  if a pin is set to output, the value
	 * set on the pin is not mirrored by the chip into SRAM, but the chip
	 * will use the value from SRAM to set the value of the pin.  What this
	 * means is that we have to learn the value from the GPIO config and
	 * make sure it is set properly when updating SRAM. */

	if (current_gpio_cfg_res.gp0_pin_dir == MCP2221_GPIO_CFG_DIR_OUTPUT) {
		if (current_gpio_cfg_res.gp0_pin_value == 1) {
			set_sram_req.gp0_settings |=
			    MCP2221_SRAM_GPIO_OUTPUT_HIGH;
		} else {
			set_sram_req.gp0_settings &=
			    ~MCP2221_SRAM_GPIO_OUTPUT_HIGH;
		}
	}
	if (current_gpio_cfg_res.gp1_pin_dir == MCP2221_GPIO_CFG_DIR_OUTPUT) {
		if (current_gpio_cfg_res.gp1_pin_value == 1) {
			set_sram_req.gp1_settings |=
			    MCP2221_SRAM_GPIO_OUTPUT_HIGH;
		} else {
			set_sram_req.gp1_settings &=
			    ~MCP2221_SRAM_GPIO_OUTPUT_HIGH;
		}
	}
	if (current_gpio_cfg_res.gp2_pin_dir == MCP2221_GPIO_CFG_DIR_OUTPUT) {
		if (current_gpio_cfg_res.gp2_pin_value == 1) {
			set_sram_req.gp2_settings |=
			    MCP2221_SRAM_GPIO_OUTPUT_HIGH;
		} else {
			set_sram_req.gp2_settings &=
			    ~MCP2221_SRAM_GPIO_OUTPUT_HIGH;
		}
	}
	if (current_gpio_cfg_res.gp3_pin_dir == MCP2221_GPIO_CFG_DIR_OUTPUT) {
		if (current_gpio_cfg_res.gp3_pin_value == 1) {
			set_sram_req.gp3_settings |=
			    MCP2221_SRAM_GPIO_OUTPUT_HIGH;
		} else {
			set_sram_req.gp3_settings &=
			    ~MCP2221_SRAM_GPIO_OUTPUT_HIGH;
		}
	}
	err = mcp2221_put_sram(sc, &set_sram_req, &set_sram_res);
	if (err)
		goto out;

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *) & set_sram_res, MCP2221_RES_BUFFER_SIZE,
	    "mcp2221_gpio_pin_ctl set sram buffer copy");
	if (set_sram_res.cmd != MCP2221_CMD_SET_SRAM ||
	    set_sram_res.completion != MCP2221_CMD_COMPLETE_OK) {
		device_printf(sc->sc_dev, "mcp2221_gpio_pin_ctl:"
		    " not the command desired, or error: %02x %02x\n",
		    set_sram_res.cmd,
		    set_sram_res.completion);
		err = EIO;
		goto out;
	}
	sc->sc_gpio_pins[pin].pin_flags = flags;
	err = 0;

out:
	return err;
}

void
umcpmio_gpio_pin_ctl(void *arg, int pin, int flags)
{
	struct umcpmio_softc *sc = arg;

	if (sc->sc_dying)
		return;

	mutex_enter(&sc->sc_action_mutex);
	if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2210)
		mcp2210_gpio_pin_ctl(sc, pin, flags);
	if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2221)
		mcp2221_gpio_pin_ctl(sc, pin, flags);
	mutex_exit(&sc->sc_action_mutex);
}

int
mcp2210_get_gp6_counter(struct umcpmio_softc *sc, int *counter,
    uint8_t reset)
{
	struct mcp2210_get_gp6_events_req req;
	struct mcp2210_get_gp6_events_res res;
	int err = 0;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	memset(&req, 0, MCP2210_REQ_BUFFER_SIZE);
	req.cmd = MCP2210_CMD_GET_GP6_EVENTS;
	req.reset_counter = reset;

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *) & req, MCP2210_REQ_BUFFER_SIZE,
	    "mcp2210_get_gp6_counter req");

	err = umcpmio_send_report(sc,
	    (uint8_t *) & req, MCP2210_REQ_BUFFER_SIZE,
	    (uint8_t *) & res, sc->sc_cv_wait);

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *) & res, MCP2210_RES_BUFFER_SIZE,
	    "mcp2210_get_gp6_counter res");

	err = mcp2210_decode_errors(req.cmd, err, res.completion);
	if (!err) {
		*counter = (res.counter_msb << 8) | res.counter_lsb;
	}
	return err;
}

static int
umcpmio_extract_gpio_sram(struct umcpmio_softc *sc,
    uint8_t *extract)
{
	int err = 0;
	struct mcp2210_get_gpio_sram_res mcp2210_get_gpio_sram_res;
	struct mcp2210_get_gpio_dir_res mcp2210_get_gpio_dir_sram_res;
	struct mcp2221_get_sram_res mcp2221_get_sram_res;

	if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2210) {
		mutex_enter(&sc->sc_action_mutex);
		err = mcp2210_get_gpio_sram(sc, &mcp2210_get_gpio_sram_res);
		mutex_exit(&sc->sc_action_mutex);
		if (err)
			return err;

		umcpmio_dump_buffer(sc->sc_dumpbuffer,
		    (uint8_t *) & mcp2210_get_gpio_sram_res, MCP2210_RES_BUFFER_SIZE,
		    "umcpmio_extract_gpio_sram mcp2210 get gpio sram buffer copy");

		extract[0] = mcp2210_get_gpio_sram_res.gp0_designation;
		extract[1] = mcp2210_get_gpio_sram_res.gp1_designation;
		extract[2] = mcp2210_get_gpio_sram_res.gp2_designation;
		extract[3] = mcp2210_get_gpio_sram_res.gp3_designation;
		extract[4] = mcp2210_get_gpio_sram_res.gp4_designation;
		extract[5] = mcp2210_get_gpio_sram_res.gp5_designation;
		extract[6] = mcp2210_get_gpio_sram_res.gp6_designation;
		extract[7] = mcp2210_get_gpio_sram_res.gp7_designation;
		extract[8] = mcp2210_get_gpio_sram_res.gp8_designation;

		mutex_enter(&sc->sc_action_mutex);
		err = mcp2210_get_gpio_dir_sram(sc, &mcp2210_get_gpio_dir_sram_res);
		mutex_exit(&sc->sc_action_mutex);
		if (err)
			return err;

		umcpmio_dump_buffer(sc->sc_dumpbuffer,
		    (uint8_t *) & mcp2210_get_gpio_dir_sram_res, MCP2210_RES_BUFFER_SIZE,
		    "umcpmio_extract_gpio_sram mcp2210 get gpio dir sram buffer copy");

		extract[9] = mcp2210_get_gpio_dir_sram_res.pin_dir_lsb;
		extract[10] = mcp2210_get_gpio_dir_sram_res.pin_dir_msb;
		extract[11] = mcp2210_get_gpio_sram_res.other_settings;
	}
	if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2221) {
		mutex_enter(&sc->sc_action_mutex);
		err = mcp2221_get_sram(sc, &mcp2221_get_sram_res);
		mutex_exit(&sc->sc_action_mutex);
		if (err)
			return err;

		umcpmio_dump_buffer(sc->sc_dumpbuffer,
		    (uint8_t *) & mcp2221_get_sram_res, MCP2221_RES_BUFFER_SIZE,
		    "umcpmio_extract_gpio_sram mcp2221 get sram buffer copy");

		extract[0] = mcp2221_get_sram_res.gp0_settings;
		extract[1] = mcp2221_get_sram_res.gp1_settings;
		extract[2] = mcp2221_get_sram_res.gp2_settings;
		extract[3] = mcp2221_get_sram_res.gp3_settings;
	}
	return err;
}

void
umcpmio_gpio_attach(struct umcpmio_softc *sc)
{
	int err;
	struct gpiobus_attach_args gba;
	uint8_t extract[UMCPMIO_MAX_GPIO_PINS + 3];

	err = umcpmio_extract_gpio_sram(sc, extract);
	if (err) {
		aprint_error_dev(sc->sc_dev, "umcpmio_gpio_attach:"
		    " extract gpio from sram: err=%d\n",
		    err);
		return;
	}

	/* The MCP2221 / MCP2221A has a pin that can have gpio interrupt
	 * ability, but there are problems with making use of it as the
	 * gpio framework runs with spin locks or hard interrupt level,
	 * and you can't call into the USB framework in that state.
	 *
	 * It is largely the same reason using the umcpmio gpio pins
	 * as attachments to gpiopps or gpioow doesn't work.  Spin
	 * locks are held there too.
	 */
	for (int c = 0; c < sc->sc_chipinfo->num_gpio_pins; c++){
		sc->sc_gpio_pins[c].pin_num = c;
		sc->sc_gpio_pins[c].pin_caps = sc->sc_chipinfo->gpio_pin_ability[c];
		if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2210) {
			sc->sc_gpio_pins[c].pin_flags =
			    mcp2210_sram_gpio_to_flags(extract, c);
		}
		if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2221) {
			sc->sc_gpio_pins[c].pin_flags =
			    mcp2221_sram_gpio_to_flags(extract[c]);
		}
		sc->sc_gpio_pins[c].pin_intrcaps = 0;
		strncpy(sc->sc_gpio_pins[c].pin_defname,
		    sc->sc_chipinfo->gpio_names[c],
		    strlen(sc->sc_chipinfo->gpio_names[c]) + 1);
	}

	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = umcpmio_gpio_pin_read;
	sc->sc_gpio_gc.gp_pin_write = umcpmio_gpio_pin_write;
	sc->sc_gpio_gc.gp_pin_ctl = umcpmio_gpio_pin_ctl;

	gba.gba_gc = &sc->sc_gpio_gc;
	gba.gba_pins = sc->sc_gpio_pins;
	gba.gba_npins = sc->sc_chipinfo->num_gpio_pins;

	sc->sc_gpio_dev = config_found(sc->sc_dev, &gba, gpiobus_print,
	    CFARGS(.iattr = "gpiobus"));
}

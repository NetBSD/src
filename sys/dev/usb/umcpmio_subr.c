/*	$NetBSD: umcpmio_subr.c,v 1.3 2025/03/25 20:38:27 riastradh Exp $	*/

/*
 * Copyright (c) 2024 Brad Spencer <brad@anduin.eldar.org>
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
__KERNEL_RCSID(0, "$NetBSD: umcpmio_subr.c,v 1.3 2025/03/25 20:38:27 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/types.h>

#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/vnode.h>

#include <dev/hid/hid.h>

#include <dev/usb/uhidev.h>
#include <dev/usb/usb.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbhid.h>

#include <dev/usb/umcpmio.h>
#include <dev/usb/umcpmio_subr.h>
#include <dev/usb/umcpmio_hid_reports.h>

int umcpmio_send_report(struct umcpmio_softc *, uint8_t *, size_t, uint8_t *,
    int);

#define UMCPMIO_DEBUG 1
#ifdef UMCPMIO_DEBUG
#define DPRINTF(x)	do { if (umcpmiodebug) printf x; } while (0)
#define DPRINTFN(n, x)	do { if (umcpmiodebug > (n)) printf x; } while (0)
extern int	umcpmiodebug;
#else
#define DPRINTF(x)	__nothing
#define DPRINTFN(n,x)	__nothing
#endif

/* Handy functions that do a bunch of things for the main driver code */

int
umcpmio_get_status(struct umcpmio_softc *sc,
    struct mcp2221_status_res *res, bool takemutex)
{
	struct mcp2221_status_req req;
	int err = 0;

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	req.cmd = MCP2221_CMD_STATUS;

	if (takemutex)
		mutex_enter(&sc->sc_action_mutex);
	err = umcpmio_send_report(sc,
	    (uint8_t *)&req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *)res, sc->sc_cv_wait);
	if (takemutex)
		mutex_exit(&sc->sc_action_mutex);

	return err;
}

void
umcpmio_set_i2c_speed(struct mcp2221_status_req *req, int flags)
{
	int i2cbaud = MCP2221_DEFAULT_I2C_SPEED;

	if (flags & I2C_F_SPEED)
		i2cbaud = 400000;

	req->set_i2c_speed = MCP2221_I2C_SET_SPEED;
	if (i2cbaud <= 0)
		i2cbaud = MCP2221_DEFAULT_I2C_SPEED;

	/*
	 * Everyone and their brother seems to store the I2C divider like this,
	 * so do likewise
	 */
	req->i2c_clock_divider = (MCP2221_INTERNAL_CLOCK / i2cbaud) - 3;
}

int
umcpmio_put_status(struct umcpmio_softc *sc,
    struct mcp2221_status_req *req, struct mcp2221_status_res *res,
    bool takemutex)
{
	int err = 0;

	req->cmd = MCP2221_CMD_STATUS;

	if (takemutex)
		mutex_enter(&sc->sc_action_mutex);
	err = umcpmio_send_report(sc,
	    (uint8_t *)req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *)res, sc->sc_cv_wait);
	if (takemutex)
		mutex_exit(&sc->sc_action_mutex);

	return err;
}

int
umcpmio_set_i2c_speed_one(struct umcpmio_softc *sc,
    int flags, bool takemutex)
{
	int err = 0;
	struct mcp2221_status_req req;
	struct mcp2221_status_res res;

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	umcpmio_set_i2c_speed(&req, flags);
	err = umcpmio_put_status(sc, &req, &res, takemutex);
	if (err)
		goto out;
	if (res.set_i2c_speed == MCP2221_I2C_SPEED_BUSY)
		err = EBUSY;
out:
	return err;
}

int
umcpmio_get_sram(struct umcpmio_softc *sc,
    struct mcp2221_get_sram_res *res, bool takemutex)
{
	struct mcp2221_get_sram_req req;
	int err = 0;

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	req.cmd = MCP2221_CMD_GET_SRAM;

	if (takemutex)
		mutex_enter(&sc->sc_action_mutex);
	err = umcpmio_send_report(sc,
	    (uint8_t *)&req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *)res, sc->sc_cv_wait);
	if (takemutex)
		mutex_exit(&sc->sc_action_mutex);

	return err;
}

int
umcpmio_put_sram(struct umcpmio_softc *sc,
    struct mcp2221_set_sram_req *req, struct mcp2221_set_sram_res *res,
    bool takemutex)
{
	int err = 0;

	req->cmd = MCP2221_CMD_SET_SRAM;

	if (takemutex)
		mutex_enter(&sc->sc_action_mutex);
	err = umcpmio_send_report(sc,
	    (uint8_t *)req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *)res, sc->sc_cv_wait);
	if (takemutex)
		mutex_exit(&sc->sc_action_mutex);

	return err;
}

/* We call the dedicated function ALT3 everywhere */

uint32_t
umcpmio_sram_gpio_to_flags(uint8_t gp_setting)
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

void
umcpmio_set_gpio_value_sram(struct mcp2221_set_sram_req *req, int pin,
    bool value)
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
			if (value)
				*newvalue |= MCP2221_SRAM_GPIO_HIGH;
			else
				*newvalue &= ~MCP2221_SRAM_GPIO_HIGH;
		}
	}
}

void
umcpmio_set_gpio_dir_sram(struct mcp2221_set_sram_req *req, int pin, int flags)
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

void
umcpmio_set_gpio_designation_sram(struct mcp2221_set_sram_req *req, int pin,
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

			if (flags & (GPIO_PIN_OUTPUT|GPIO_PIN_INPUT)) {
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
					/*
					 * ALT3 will always be used as
					 * the dedicated function
					 * specific to the pin.  Not
					 * all of the pins will have
					 * the alt functions below #3.
					 */
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

void
umcpmio_set_gpio_irq_sram(struct mcp2221_set_sram_req *req, int irqmode)
{
	req->alter_gpio_config = MCP2221_SRAM_ALTER_GPIO;

	if (irqmode & (GPIO_INTR_POS_EDGE | GPIO_INTR_DOUBLE_EDGE)) {
		req->irq_config |= MCP2221_SRAM_ALTER_IRQ |
		    MCP2221_SRAM_ALTER_POS_EDGE |
		    MCP2221_SRAM_ENABLE_POS_EDGE |
		    MCP2221_SRAM_CLEAR_IRQ;
	}
	if (irqmode & (GPIO_INTR_NEG_EDGE | GPIO_INTR_DOUBLE_EDGE)) {
		req->irq_config |= MCP2221_SRAM_ALTER_IRQ |
		    MCP2221_SRAM_ALTER_NEG_EDGE |
		    MCP2221_SRAM_ENABLE_NEG_EDGE |
		    MCP2221_SRAM_CLEAR_IRQ;
	}

	if (req->irq_config != 0) {
		req->gp1_settings = MCP2221_SRAM_PIN_IS_ALT2;
	} else {
		req->irq_config = MCP2221_SRAM_ALTER_IRQ |
		    MCP2221_SRAM_CLEAR_IRQ;
		req->gp1_settings = MCP2221_SRAM_PIN_IS_GPIO |
		    MCP2221_SRAM_GPIO_INPUT;
	}
}

/*
 * It is unfortunate that the GET and PUT requests are not symertric.  That is,
 * the bits sort of line up but not quite between a GET and PUT.
 */

static struct umcpmio_mapping_put umcpmio_vref_puts[] = {
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
umcpmio_set_dac_vref(struct mcp2221_set_sram_req *req, char *newvref)
{
	int i;

	for (i = 0; i < __arraycount(umcpmio_vref_puts); i++) {
		if (strncmp(newvref, umcpmio_vref_puts[i].tname,
		    UMCPMIO_VREF_NAME) == 0) {
			break;
		}
	}

	if (i == __arraycount(umcpmio_vref_puts))
		return;

	req->dac_voltage_reference |= umcpmio_vref_puts[i].mask |
	    MCP2221_SRAM_CHANGE_DAC_VREF;
}

int
umcpmio_set_dac_vref_one(struct umcpmio_softc *sc, char *newvref,
    bool takemutex)
{
	struct mcp2221_set_sram_req req;
	struct mcp2221_set_sram_res res;
	int err = 0;

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	umcpmio_set_dac_vref(&req, newvref);
	err = umcpmio_put_sram(sc, &req, &res, takemutex);

	return err;
}

void
umcpmio_set_dac_value(struct mcp2221_set_sram_req *req, uint8_t newvalue)
{
	req->set_dac_output_value |= (newvalue & MCP2221_SRAM_DAC_VALUE_MASK) |
	    MCP2221_SRAM_CHANGE_DAC_VREF;
}

int
umcpmio_set_dac_value_one(struct umcpmio_softc *sc, uint8_t newvalue,
    bool takemutex)
{
	struct mcp2221_set_sram_req req;
	struct mcp2221_set_sram_res res;
	int err = 0;

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	umcpmio_set_dac_value(&req, newvalue);
	err = umcpmio_put_sram(sc, &req, &res, takemutex);

	return err;
}

void
umcpmio_set_adc_vref(struct mcp2221_set_sram_req *req, char *newvref)
{
	int i;

	for (i = 0; i < __arraycount(umcpmio_vref_puts); i++) {
		if (strncmp(newvref, umcpmio_vref_puts[i].tname,
		    UMCPMIO_VREF_NAME) == 0) {
			break;
		}
	}

	if (i == __arraycount(umcpmio_vref_puts))
		return;

	req->adc_voltage_reference |= umcpmio_vref_puts[i].mask |
	    MCP2221_SRAM_CHANGE_ADC_VREF;
}

int
umcpmio_set_adc_vref_one(struct umcpmio_softc *sc, char *newvref,
    bool takemutex)
{
	struct mcp2221_set_sram_req req;
	struct mcp2221_set_sram_res res;
	int err = 0;

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	umcpmio_set_adc_vref(&req, newvref);
	err = umcpmio_put_sram(sc, &req, &res, takemutex);

	return err;
}

static struct umcpmio_mapping_put umcpmio_dc_puts[] = {
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
umcpmio_set_gpioclock_dc(struct mcp2221_set_sram_req *req, char *new_dc)
{
	int i;

	for (i = 0; i < __arraycount(umcpmio_dc_puts); i++) {
		if (strncmp(new_dc, umcpmio_dc_puts[i].tname,
		    UMCPMIO_VREF_NAME) == 0) {
			break;
		}
	}

	if (i == __arraycount(umcpmio_dc_puts))
		return;

	req->clock_output_divider |= umcpmio_dc_puts[i].mask;
}

int
umcpmio_set_gpioclock_dc_one(struct umcpmio_softc *sc, char *new_dutycycle,
    bool takemutex)
{
	struct mcp2221_get_sram_res current_sram_res;
	struct mcp2221_set_sram_req req;
	struct mcp2221_set_sram_res res;
	int err = 0;

	err = umcpmio_get_sram(sc, &current_sram_res, takemutex);
	if (err)
		goto out;

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	umcpmio_set_gpioclock_dc(&req, new_dutycycle);
	DPRINTF(("umcpmio_set_gpioclock_dc_one:"
		" req.clock_output_divider=%02x, current mask=%02x\n",
		req.clock_output_divider,
		(current_sram_res.clock_divider &
		    MCP2221_SRAM_GPIO_CLOCK_CD_MASK)));
	req.clock_output_divider |=
	    (current_sram_res.clock_divider &
		MCP2221_SRAM_GPIO_CLOCK_CD_MASK) |
	    MCP2221_SRAM_GPIO_CHANGE_DCCD;
	DPRINTF(("umcpmio_set_gpioclock_dc_one:"
		" SET req.clock_output_divider=%02x\n",
		req.clock_output_divider));
	err = umcpmio_put_sram(sc, &req, &res, takemutex);
out:
	return err;
}

static struct umcpmio_mapping_put umcpmio_cd_puts[] = {
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
umcpmio_set_gpioclock_cd(struct mcp2221_set_sram_req *req, char *new_cd)
{
	int i;

	for (i = 0; i < __arraycount(umcpmio_cd_puts); i++) {
		if (strncmp(new_cd, umcpmio_cd_puts[i].tname,
		    UMCPMIO_CD_NAME) == 0) {
			break;
		}
	}

	if (i == __arraycount(umcpmio_cd_puts))
		return;

	req->clock_output_divider |= umcpmio_cd_puts[i].mask;
}

int
umcpmio_set_gpioclock_cd_one(struct umcpmio_softc *sc, char *new_clockdivider,
    bool takemutex)
{
	struct mcp2221_get_sram_res current_sram_res;
	struct mcp2221_set_sram_req req;
	struct mcp2221_set_sram_res res;
	int err = 0;

	err = umcpmio_get_sram(sc, &current_sram_res, takemutex);
	if (err)
		goto out;

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	umcpmio_set_gpioclock_cd(&req, new_clockdivider);
	DPRINTF(("umcpmio_set_gpioclock_cd_one:"
		" req.clock_output_divider=%02x, current mask=%02x\n",
		req.clock_output_divider,
		(current_sram_res.clock_divider &
		    MCP2221_SRAM_GPIO_CLOCK_CD_MASK)));
	req.clock_output_divider |=
	    (current_sram_res.clock_divider &
		MCP2221_SRAM_GPIO_CLOCK_DC_MASK) |
	    MCP2221_SRAM_GPIO_CHANGE_DCCD;
	DPRINTF(("umcpmio_set_gpioclock_cd_one:"
		" SET req.clock_output_divider=%02x\n",
		req.clock_output_divider));
	err = umcpmio_put_sram(sc, &req, &res, takemutex);
out:
	return err;
}

int
umcpmio_get_gpio_cfg(struct umcpmio_softc *sc,
    struct mcp2221_get_gpio_cfg_res *res, bool takemutex)
{
	struct mcp2221_get_gpio_cfg_req req;
	int err = 0;

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	req.cmd = MCP2221_CMD_GET_GPIO_CFG;

	if (takemutex)
		mutex_enter(&sc->sc_action_mutex);
	err = umcpmio_send_report(sc,
	    (uint8_t *)&req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *)res, sc->sc_cv_wait);
	if (takemutex)
		mutex_exit(&sc->sc_action_mutex);

	return err;
}

int
umcpmio_put_gpio_cfg(struct umcpmio_softc *sc,
    struct mcp2221_set_gpio_cfg_req *req, struct mcp2221_set_gpio_cfg_res *res,
    bool takemutex)
{
	int err = 0;

	req->cmd = MCP2221_CMD_SET_GPIO_CFG;

	if (takemutex)
		mutex_enter(&sc->sc_action_mutex);
	err = umcpmio_send_report(sc,
	    (uint8_t *)req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *)res, sc->sc_cv_wait);
	if (takemutex)
		mutex_exit(&sc->sc_action_mutex);

	return err;
}

/* So... if the pin isn't set to GPIO, just call the output LOW */

int
umcpmio_get_gpio_value(struct umcpmio_softc *sc,
    int pin, bool takemutex)
{
	struct mcp2221_get_gpio_cfg_res get_gpio_cfg_res;
	int err = 0;
	int r = GPIO_PIN_LOW;

	err = umcpmio_get_gpio_cfg(sc, &get_gpio_cfg_res, takemutex);
	if (err)
		goto out;

	if (get_gpio_cfg_res.cmd != MCP2221_CMD_GET_GPIO_CFG ||
	    get_gpio_cfg_res.completion != MCP2221_CMD_COMPLETE_OK) {
		device_printf(sc->sc_dev, "umcpmio_get_gpio_value:"
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

void
umcpmio_set_gpio_value(struct mcp2221_set_gpio_cfg_req *req,
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

int
umcpmio_set_gpio_value_one(struct umcpmio_softc *sc,
    int pin, bool value, bool takemutex)
{
	int err = 0;
	struct mcp2221_set_gpio_cfg_req req;
	struct mcp2221_set_gpio_cfg_res res;

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	umcpmio_set_gpio_value(&req, pin, value);
	err = umcpmio_put_gpio_cfg(sc, &req, &res, takemutex);
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

int
umcpmio_get_flash(struct umcpmio_softc *sc, uint8_t subcode,
    struct mcp2221_get_flash_res *res, bool takemutex)
{
	struct mcp2221_get_flash_req req;
	int err = 0;

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	req.cmd = MCP2221_CMD_GET_FLASH;

	if (subcode < MCP2221_FLASH_SUBCODE_CS ||
	    subcode > MCP2221_FLASH_SUBCODE_CHIPSN)
		return EINVAL;

	req.subcode = subcode;

	if (takemutex)
		mutex_enter(&sc->sc_action_mutex);
	err = umcpmio_send_report(sc,
	    (uint8_t *)&req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *)res, sc->sc_cv_wait);
	if (takemutex)
		mutex_exit(&sc->sc_action_mutex);

	return err;
}

int
umcpmio_put_flash(struct umcpmio_softc *sc, struct mcp2221_put_flash_req *req,
    struct mcp2221_put_flash_res *res, bool takemutex)
{
	int err = 0;

	req->cmd = MCP2221_CMD_SET_FLASH;

	if (req->subcode < MCP2221_FLASH_SUBCODE_CS ||
	    req->subcode > MCP2221_FLASH_SUBCODE_CHIPSN) {
		DPRINTF(("umcpmio_put_flash: subcode out of range:"
			" subcode=%d\n",
			req->subcode));
		return EINVAL;
	}

	if (takemutex)
		mutex_enter(&sc->sc_action_mutex);
	err = umcpmio_send_report(sc,
	    (uint8_t *)req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *)res, sc->sc_cv_wait);
	if (takemutex)
		mutex_exit(&sc->sc_action_mutex);

	return err;
}

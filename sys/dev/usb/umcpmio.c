/*	$NetBSD: umcpmio.c,v 1.9 2025/12/12 17:49:35 andvar Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: umcpmio.c,v 1.9 2025/12/12 17:49:35 andvar Exp $");

/*
 * Driver for the Microchip MCP2221 / MCP2221A USB multi-io chip
 *
 * https://www.microchip.com/en-us/product/MCP2210
 * https://www.microchip.com/en-us/product/MCP2221
 *
 */

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/types.h>

#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/gpio.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/vnode.h>

#include <dev/usb/umcpmio.h>
#include <dev/usb/umcpmio_info.h>
#include <dev/usb/umcpmio_hid_reports.h>
#include <dev/usb/umcpmio_io.h>
#include <dev/usb/umcpmio_transport.h>
#include <dev/usb/umcpmio_gpio.h>
#include <dev/usb/umcpmio_spi.h>
#include <dev/usb/umcpmio_iic.h>
#include <dev/usb/umcpmio_subr.h>

static const struct usb_devno umcpmio_devs[] = {
	{USB_VENDOR_MICROCHIP, USB_PRODUCT_MICROCHIP_MCP2210},
	{USB_VENDOR_MICROCHIP, USB_PRODUCT_MICROCHIP_MCP2221},
};
#define umcpmio_lookup(v, p) usb_lookup(umcpmio_devs, v, p)

static int umcpmio_match(device_t, cfdata_t, void *);
static void umcpmio_attach(device_t, device_t, void *);
static int umcpmio_detach(device_t, int);
static int umcpmio_activate(device_t, enum devact);
static int umcpmio_verify_sysctl(SYSCTLFN_ARGS);
static int mcp2221_verify_dac_sysctl(SYSCTLFN_ARGS);
static int mcp2221_verify_adc_sysctl(SYSCTLFN_ARGS);
static int mcp2221_verify_gpioclock_dc_sysctl(SYSCTLFN_ARGS);
static int mcp2221_verify_gpioclock_cd_sysctl(SYSCTLFN_ARGS);
static int mcp2210_verify_counter_sysctl(SYSCTLFN_ARGS);
static int mcp2210_reset_counter_sysctl(SYSCTLFN_ARGS);

#define UMCPMIO_DEBUG 1
#ifdef UMCPMIO_DEBUG
#define DPRINTF(x)	do { if (umcpmiodebug) printf x; } while (0)
#define DPRINTFN(n, x)	do { if (umcpmiodebug > (n)) printf x; } while (0)
int umcpmiodebug = 0;
#else
#define DPRINTF(x)	__nothing
#define DPRINTFN(n, x)	__nothing
#endif

CFATTACH_DECL_NEW(umcpmio, sizeof(struct umcpmio_softc), umcpmio_match,
    umcpmio_attach, umcpmio_detach, umcpmio_activate);

extern struct cfdriver umcpmio_cd;

static dev_type_open(umcpmio_dev_open);
static dev_type_read(umcpmio_dev_read);
static dev_type_write(umcpmio_dev_write);
static dev_type_close(umcpmio_dev_close);
static dev_type_ioctl(umcpmio_dev_ioctl);

const struct cdevsw umcpmio_cdevsw = {
	.d_open = umcpmio_dev_open,
	.d_close = umcpmio_dev_close,
	.d_read = umcpmio_dev_read,
	.d_write = umcpmio_dev_write,
	.d_ioctl = umcpmio_dev_ioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

static const char umcpmio_valid_vrefs[] =
"4.096V, 2.048V, 1.024V, OFF, VDD";

static const char umcpmio_valid_dcs[] =
"75%, 50%, 25%, 0%";

static const char umcpmio_valid_cds[] =
"375kHz, 750kHz, 1.5MHz, 3MHz, 6MHz, 12MHz, 24MHz";


/* Accessing the ADC, DAC or EEPROM part of the chip */

#define UMCPMIO_DEV_UNIT(m) ((m) & 0x100 ? ((m) & 0xff) : (m) & 0x80 ? ((m) & 0x7f) / 3 : (m))
#define UMCPMIO_DEV_WHAT(m) ((m) & 0x100 ? EEPROM_DEV : (m) & 0x80 ? (((m) & 0x7f) % 3) + 1 : CONTROL_DEV)


static int
umcpmio_dev_open(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct umcpmio_softc *sc;
	int dunit;
	int pin = -1;
	int error = 0;

	sc = device_lookup_private(&umcpmio_cd, UMCPMIO_DEV_UNIT(minor(dev)));
	if (!sc)
		return ENXIO;

	dunit = UMCPMIO_DEV_WHAT(minor(dev));

	if (sc->sc_dev_open[dunit]) {
		DPRINTF(("umcpmio_dev_open: dunit=%d BUSY\n", dunit));
		return EBUSY;
	}

	/* The control device only allows for ioctl calls, so pretty much allow
	 * any sort of access.  For the ADC, you perform a strict O_RDONLY and
	 * for the DAC a strict O_WRONLY.  It is an error to try and do a
	 * O_RDWR It makes little sense to try and support select or poll.  The
	 * ADC and DAC are always available for use. */
	if (dunit != CONTROL_DEV &&
	    ((flags & FREAD) && (flags & FWRITE))) {
		DPRINTF(("umcpmio_dev_open: Not CONTROL device and trying to"
		    " do READ and WRITE\n"));
		return EINVAL;
	}
	error = 0;

	mutex_enter(&sc->sc_action_mutex);

	if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2210) {
		if (dunit != CONTROL_DEV &&
		    dunit != EEPROM_DEV)
			error = EINVAL;
	}
	if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2221) {
		if (dunit != CONTROL_DEV) {
			switch (dunit) {
			case GP1_DEV:
				pin = 1;
				break;
			case GP2_DEV:
				pin = 2;
				break;
			case GP3_DEV:
				pin = 3;
				break;
			default:
				error = EINVAL;
			}

			if (!error) {
				if (flags & FREAD) {
					sc->sc_adcdac_pin_flags[pin] = sc->sc_gpio_pins[pin].pin_flags;
					error = mcp2221_gpio_pin_ctl(sc, pin,
					    GPIO_PIN_ALT0);
				} else {
					if (pin == 1) {
						error = EINVAL;
					} else {
						sc->sc_adcdac_pin_flags[pin] =
						    sc->sc_gpio_pins[pin].pin_flags;
						error = mcp2221_gpio_pin_ctl(sc,
						    pin, GPIO_PIN_ALT1);
					}
				}
			}
		}
	}
	if (!error)
		sc->sc_dev_open[dunit] = true;

	mutex_exit(&sc->sc_action_mutex);

	DPRINTF(("umcpmio_dev_open: Opened dunit=%d, pin=%d, error=%d\n",
	    dunit, pin, error));

	return error;
}
/* Read the ADC on the MCP2221 / MCP2221A */

static int
umcpmio_dev_read_adc(dev_t dev, struct uio *uio, int flags,
    int dunit, struct umcpmio_softc *sc)
{
	struct mcp2221_status_res status_res;
	int error = 0;
	uint8_t adc_lsb;
	uint8_t adc_msb;
	uint16_t buf;

	while (uio->uio_resid && !sc->sc_dying) {
		mutex_enter(&sc->sc_action_mutex);
		error = mcp2221_get_status(sc, &status_res);
		mutex_exit(&sc->sc_action_mutex);
		if (error)
			break;
		switch (dunit) {
		case GP1_DEV:
			adc_lsb = status_res.adc_channel0_lsb;
			adc_msb = status_res.adc_channel0_msb;
			break;
		case GP2_DEV:
			adc_lsb = status_res.adc_channel1_lsb;
			adc_msb = status_res.adc_channel1_msb;
			break;
		case GP3_DEV:
			adc_lsb = status_res.adc_channel2_lsb;
			adc_msb = status_res.adc_channel2_msb;
			break;
		default:
			error = EINVAL;
			break;
		}
		if (error)
			break;
		if (sc->sc_dying)
			break;

		buf = adc_msb << 8;
		buf |= adc_lsb;
		error = uiomove(&buf, 2, uio);
		if (error)
			break;
	}

	return error;
}
/* Read the EEPROM on the MCP2210 */

static int
umcpmio_dev_read_eeprom(dev_t dev, struct uio *uio, int flags,
    struct umcpmio_softc *sc)
{
	int error;

	/* We do not make this an error.  There is nothing wrong with running
	 * off the end here, just return EOF. */
	if (uio->uio_offset > 0xff)
		return 0;

	while (uio->uio_resid &&
	    uio->uio_offset <= 0xff &&
	    !sc->sc_dying) {
		uint8_t buf;
		int reg_addr = uio->uio_offset;

		if ((error = mcp2210_read_eeprom(sc, reg_addr, &buf)) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "%s: read failed at 0x%02x: %d\n",
			    __func__, reg_addr, error);
			return error;
		}
		if (sc->sc_dying)
			break;

		if ((error = uiomove(&buf, 1, uio)) != 0)
			return error;
	}

	if (sc->sc_dying) {
		return EIO;
	}
	return 0;
}

static int
umcpmio_dev_read(dev_t dev, struct uio *uio, int flags)
{
	struct umcpmio_softc *sc;
	int dunit;

	sc = device_lookup_private(&umcpmio_cd, UMCPMIO_DEV_UNIT(minor(dev)));
	if (sc == NULL)
		return ENXIO;

	dunit = UMCPMIO_DEV_WHAT(minor(dev));

	if (dunit == CONTROL_DEV) {
		return EINVAL;
	}
	if (dunit != EEPROM_DEV)
		return (umcpmio_dev_read_adc(dev, uio, flags, dunit, sc));
	else
		return (umcpmio_dev_read_eeprom(dev, uio, flags, sc));
}

/* Write to the DAC on the MCP2221 / MCP2221A*/

static int
umcpmio_dev_write_dac(dev_t dev, struct uio *uio, int flags,
    struct umcpmio_softc *sc)
{
	int error = 0;

	while (uio->uio_resid && !sc->sc_dying) {
		uint8_t buf;

		if ((error = uiomove(&buf, 1, uio)) != 0)
			break;

		if (sc->sc_dying)
			break;

		mutex_enter(&sc->sc_action_mutex);
		error = mcp2221_set_dac_value_one(sc, buf);
		mutex_exit(&sc->sc_action_mutex);
		if (error)
			break;
	}

	return error;
}

/* Write to the EEPROM on the MCP2210 */

static int
umcpmio_dev_write_eeprom(dev_t dev, struct uio *uio, int flags,
    struct umcpmio_softc *sc)
{
	int error;

	/* Same thing as read, this is not considered an error */
	if (uio->uio_offset > 0xff)
		return 0;

	while (uio->uio_resid &&
	    uio->uio_offset <= 0xff &&
	    !sc->sc_dying) {
		uint8_t buf;
		int reg_addr = uio->uio_offset;

		if ((error = uiomove(&buf, 1, uio)) != 0)
			break;

		if (sc->sc_dying)
			break;

		if ((error = mcp2210_write_eeprom(sc, (uint8_t) reg_addr, buf)) != 0) {
			device_printf(sc->sc_dev,
			    "%s: write failed at 0x%02x: %d\n",
			    __func__, reg_addr, error);
			return error;
		}
	}

	if (sc->sc_dying) {
		return EIO;
	}
	return error;
}

static int
umcpmio_dev_write(dev_t dev, struct uio *uio, int flags)
{
	struct umcpmio_softc *sc;
	int dunit;

	sc = device_lookup_private(&umcpmio_cd, UMCPMIO_DEV_UNIT(minor(dev)));
	if (sc == NULL)
		return ENXIO;

	dunit = UMCPMIO_DEV_WHAT(minor(dev));

	if (dunit == CONTROL_DEV) {
		return EINVAL;
	}
	if (dunit != EEPROM_DEV)
		return (umcpmio_dev_write_dac(dev, uio, flags, sc));
	else
		return (umcpmio_dev_write_eeprom(dev, uio, flags, sc));
}

/* Close everything up */

static int
umcpmio_dev_close(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct umcpmio_softc *sc;
	int dunit;
	int pin;
	int error = 0;

	sc = device_lookup_private(&umcpmio_cd, UMCPMIO_DEV_UNIT(minor(dev)));
	if (sc->sc_dying)
		return EIO;

	dunit = UMCPMIO_DEV_WHAT(minor(dev));

	mutex_enter(&sc->sc_action_mutex);

	if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2221) {
		if (dunit != CONTROL_DEV &&
		    dunit != EEPROM_DEV) {
			switch (dunit) {
			case GP1_DEV:
				pin = 1;
				break;
			case GP2_DEV:
				pin = 2;
				break;
			case GP3_DEV:
				pin = 3;
				break;
			default:
				error = EINVAL;
				goto out;
				break;
			}
			if (sc->sc_adcdac_pin_flags[pin] != -1) {
				error = mcp2221_gpio_pin_ctl(sc, pin, sc->sc_adcdac_pin_flags[pin]);
				sc->sc_adcdac_pin_flags[pin] = -1;
			} else {
				error = mcp2221_gpio_pin_ctl(sc, pin, GPIO_PIN_INPUT);
			}
		}
	}
out:
	sc->sc_dev_open[dunit] = false;
	mutex_exit(&sc->sc_action_mutex);

	return error;
}

static int
umcpmio_dev_ioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct umcpmio_softc *sc;
	union umcpmio_ioctl_get_status get_status_res;
	struct mcp2221_get_sram_res get_sram_res;
	struct mcp2221_get_gpio_cfg_res get_gpio_cfg_res;
	struct umcpmio_ioctl_get_flash get_flash_res;
	union umcpmio_ioctl_get_status *ioctl_get_status;
	struct mcp2221_get_sram_res *ioctl_get_sram;
	struct mcp2221_get_gpio_cfg_res *ioctl_get_gpio_cfg;
	struct umcpmio_ioctl_get_flash *ioctl_get_flash;
	struct umcpmio_ioctl_put_flash *ioctl_put_flash;
	struct mcp2221_put_flash_req mcp2221_put_req;
	struct mcp2221_put_flash_res mcp2221_put_res;
	struct mcp2210_ioctl_get_sram *mcp2210_get_sram;
	struct mcp2210_get_nvram_res mcp2210_gnvram_res;
	struct mcp2210_set_nvram_req mcp2210_snvram_req;
	struct mcp2210_set_nvram_res mcp2210_snvram_res;
	struct mcp2210_cancel_spi_res *ioctl_cancel_spi;
	struct mcp2210_cancel_spi_res mcp2210_cancel_spi;
	uint8_t blob_req[UMCPMIO_REQ_BUFFER_SIZE];
	uint8_t blob_res[UMCPMIO_RES_BUFFER_SIZE];
	uint8_t *chip_type;
	int dunit;
	int error = 0;

	sc = device_lookup_private(&umcpmio_cd, UMCPMIO_DEV_UNIT(minor(dev)));
	if (sc->sc_dying)
		return EIO;

	dunit = UMCPMIO_DEV_WHAT(minor(dev));

	if (dunit != CONTROL_DEV) {
		/* It actually is fine to call ioctl with a unsupported cmd,
		 * but be a little noisy if debug is enabled. */
		DPRINTF(("umcpmio_dev_ioctl: dunit is not the CONTROL device:"
		    " dunit=%d, cmd=%ld\n", dunit, cmd));
		return EINVAL;
	}
	mutex_enter(&sc->sc_action_mutex);

	switch (cmd) {
		/* The GET calls use a shadow buffer for each type of call.
		 * That probably isn't actually needed and the memcpy could be
		 * avoided.  but...  it is only ever 64 bytes, so maybe not a
		 * big deal. */

	case UMCPMIO_GET_STATUS:
		ioctl_get_status = (union umcpmio_ioctl_get_status *)data;
		switch (sc->sc_chipinfo->usb_id) {
		case USB_PRODUCT_MICROCHIP_MCP2210:
			error = mcp2210_get_status(sc, &get_status_res.mcp2210_status_res);
			break;
		case USB_PRODUCT_MICROCHIP_MCP2221:
			error = mcp2221_get_status(sc, &get_status_res.mcp2221_status_res);
			break;
		default:
			panic("UMCPMIO_GET_STATUS ioctl.  Unknown sc_chipinfo.usb_id");
		};
		umcpmio_dump_buffer(sc->sc_dumpbuffer,
		    &get_status_res.status_blob[0], UMCPMIO_RES_BUFFER_SIZE,
		    "umcpmio_dev_ioctl: UMCPMIO_GET_STATUS: get_status_res");
		DPRINTF(("umcpmio_dev_ioctl: UMCPMIO_GET_STATUS:"
		    " error=%d\n", error));
		if (error)
			break;
		memcpy(ioctl_get_status, &get_status_res,
		    UMCPMIO_RES_BUFFER_SIZE);
		break;

	case MCP2210_CANCEL_SPI:
		if (sc->sc_chipinfo->usb_id != USB_PRODUCT_MICROCHIP_MCP2210) {
			error = EINVAL;
			break;
		}
		ioctl_cancel_spi = (struct mcp2210_cancel_spi_res *)data;
		error = mcp2210_cancel_spi_transfer(sc, &mcp2210_cancel_spi);
		if (error)
			break;
		memcpy(ioctl_cancel_spi, &mcp2210_cancel_spi,
		    MCP2210_RES_BUFFER_SIZE);
		break;

	case MCP2210_GET_SRAM:
		if (sc->sc_chipinfo->usb_id != USB_PRODUCT_MICROCHIP_MCP2210) {
			error = EINVAL;
			break;
		}
		mcp2210_get_sram = (struct mcp2210_ioctl_get_sram *)data;
		memset(&blob_req[0], 0, UMCPMIO_REQ_BUFFER_SIZE);
		DPRINTF(("umcpmio_dev_ioctl: MCP2210_GET_SRAM:"
		    " cmd=%d\n", mcp2210_get_sram->cmd));
		if (mcp2210_get_sram->cmd == MCP2210_CMD_GET_GPIO_SRAM ||
		    mcp2210_get_sram->cmd == MCP2210_CMD_GET_GPIO_VAL_SRAM ||
		    mcp2210_get_sram->cmd == MCP2210_CMD_GET_GPIO_DIR_SRAM ||
		    mcp2210_get_sram->cmd == MCP2210_CMD_GET_SPI_SRAM) {
			blob_req[0] = mcp2210_get_sram->cmd;
			DPRINTF(("umcpmio_dev_ioctl: MCP2210_GET_SRAM:"
			    " blob_req[0]=%d\n", blob_req[0]));
			error = umcpmio_send_report(sc, &blob_req[0], UMCPMIO_REQ_BUFFER_SIZE,
			    &blob_res[0], sc->sc_cv_wait);
		} else {
			error = EINVAL;
		}

		DPRINTF(("umcpmio_dev_ioctl: MCP2210_GET_SRAM:"
		    " error=%d\n", error));

		if (!error)
			memcpy(mcp2210_get_sram->res, &blob_res[0], UMCPMIO_RES_BUFFER_SIZE);

		break;

	case MCP2221_GET_SRAM:
		if (sc->sc_chipinfo->usb_id != USB_PRODUCT_MICROCHIP_MCP2221) {
			error = EINVAL;
			break;
		}
		ioctl_get_sram = (struct mcp2221_get_sram_res *)data;
		error = mcp2221_get_sram(sc, &get_sram_res);
		umcpmio_dump_buffer(sc->sc_dumpbuffer,
		    (uint8_t *) & get_sram_res, MCP2221_RES_BUFFER_SIZE,
		    "umcpmio_dev_ioctl: MCP2221_GET_SRAM: get_sram_res");
		DPRINTF(("umcpmio_dev_ioctl: MCP2221_GET_SRAM:"
		    " mcp2221_get_sram error=%d\n", error));
		if (error)
			break;
		memcpy(ioctl_get_sram, &get_sram_res,
		    MCP2221_RES_BUFFER_SIZE);
		break;

	case MCP2221_GET_GP_CFG:
		if (sc->sc_chipinfo->usb_id != USB_PRODUCT_MICROCHIP_MCP2221) {
			error = EINVAL;
			break;
		}
		ioctl_get_gpio_cfg = (struct mcp2221_get_gpio_cfg_res *)data;
		error = mcp2221_get_gpio_cfg(sc, &get_gpio_cfg_res);
		umcpmio_dump_buffer(sc->sc_dumpbuffer,
		    (uint8_t *) & get_gpio_cfg_res, MCP2221_RES_BUFFER_SIZE,
		    "umcpmio_dev_ioctl: MCP2221_GET_GP_CFG: get_gpio_cfg_res");
		DPRINTF(("umcpmio_dev_ioctl: MCP2221_GET_GP_CFG:"
		    " mcp2221_get_gpio_cfg error=%d\n", error));
		if (error)
			break;
		memcpy(ioctl_get_gpio_cfg, &get_gpio_cfg_res,
		    MCP2221_RES_BUFFER_SIZE);
		break;

	case UMCPMIO_GET_FLASH:
		ioctl_get_flash = (struct umcpmio_ioctl_get_flash *)data;
		switch (sc->sc_chipinfo->usb_id) {
		case USB_PRODUCT_MICROCHIP_MCP2210:
			error = mcp2210_get_nvram(sc, ioctl_get_flash->subcode,
			    &get_flash_res.res.mcp2210_get_nvram_res);
			break;
		case USB_PRODUCT_MICROCHIP_MCP2221:
			error = mcp2221_get_flash(sc, ioctl_get_flash->subcode,
			    &get_flash_res.res.mcp2221_get_flash_res);
			break;
		default:
			panic("UMCPMIO_GET_FLASH ioctl.  Unknown sc_chipinfo.usb_id");
		};
		umcpmio_dump_buffer(sc->sc_dumpbuffer,
		    &get_flash_res.res.get_blob[0], MCP2221_RES_BUFFER_SIZE,
		    "umcpmio_dev_ioctl: UMCPMIO_GET_FLASH: get_flash_res");
		DPRINTF(("umcpmio_dev_ioctl: UMCPMIO_GET_FLASH:"
		    " subcode=%d, error=%d\n",
		    ioctl_get_flash->subcode, error));
		if (error)
			break;
		memcpy(&ioctl_get_flash->res.get_blob[0], &get_flash_res.res.get_blob[0],
		    UMCPMIO_RES_BUFFER_SIZE);
		break;

	case UMCPMIO_PUT_FLASH:
		ioctl_put_flash = (struct umcpmio_ioctl_put_flash *)data;
		DPRINTF(("umcpmio_dev_ioctl: UMCPMIO_PUT_FLASH:"
		    " subcode=%d\n",
		    ioctl_put_flash->subcode));

		if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2210) {
			/* For the MCP-2210:
			 *
			 * Always do a read-modify-write operation filling in
			 * only the values that are allowed.  We allow gpio,
			 * SPI and some USB power parameters to be changed. */
			if (ioctl_put_flash->subcode != MCP2210_NVRAM_SUBCODE_SPI &&
			    ioctl_put_flash->subcode != MCP2210_NVRAM_SUBCODE_CS &&
			    ioctl_put_flash->subcode != MCP2210_NVRAM_SUBCODE_USBKEYPARAMS) {
				error = EINVAL;
				break;
			}
			error = mcp2210_get_nvram(sc, ioctl_put_flash->subcode,
			    &mcp2210_gnvram_res);
			if (error)
				break;

			uint8_t x, y;
			switch (ioctl_put_flash->subcode) {
			case MCP2210_NVRAM_SUBCODE_SPI:
				memcpy(&mcp2210_snvram_req, &mcp2210_gnvram_res, UMCPMIO_REQ_BUFFER_SIZE);
				mcp2210_snvram_req.subcode = MCP2210_NVRAM_SUBCODE_SPI;
				memcpy(&mcp2210_snvram_req.u.spi.cs_to_data_delay_lsb,
				    &ioctl_put_flash->req.mcp2210_set_req.u.spi.cs_to_data_delay_lsb,
				    &mcp2210_snvram_req.u.spi.bytes_per_spi_transaction_msb -
				    &mcp2210_snvram_req.u.spi.cs_to_data_delay_lsb + 1);
				break;
			case MCP2210_NVRAM_SUBCODE_CS:
				memcpy(&mcp2210_snvram_req, &mcp2210_gnvram_res, UMCPMIO_REQ_BUFFER_SIZE);
				mcp2210_snvram_req.subcode = MCP2210_NVRAM_SUBCODE_CS;
				/* Make sure that the password fields are
				 * blanked out with 0 so the password can't be
				 * changed by accident. */
				memset(&mcp2210_snvram_req.u.cs.password_byte_1, 0,
				    &mcp2210_snvram_req.u.cs.password_byte_8 -
				    &mcp2210_snvram_req.u.cs.password_byte_1 + 1);
				memcpy(&mcp2210_snvram_req.u.cs.gp0_designation,
				    &ioctl_put_flash->req.mcp2210_set_req.u.cs.gp0_designation,
				    &mcp2210_snvram_req.u.cs.default_direction_msb -
				    &mcp2210_snvram_req.u.cs.gp0_designation + 1);
				break;
			case MCP2210_NVRAM_SUBCODE_USBKEYPARAMS:
				/* For this subcode, the get and set are not
				 * really symmetric, so copy the fields in one
				 * at a time. */
				mcp2210_snvram_req.subcode = MCP2210_NVRAM_SUBCODE_USBKEYPARAMS;
				mcp2210_snvram_req.u.usbkeyparams.lsb_usb_vid =
				    mcp2210_gnvram_res.u.usbkeyparams.lsb_usb_vid;
				mcp2210_snvram_req.u.usbkeyparams.msb_usb_vid =
				    mcp2210_gnvram_res.u.usbkeyparams.msb_usb_vid;
				mcp2210_snvram_req.u.usbkeyparams.lsb_usb_pid =
				    mcp2210_gnvram_res.u.usbkeyparams.lsb_usb_pid;
				mcp2210_snvram_req.u.usbkeyparams.msb_usb_pid =
				    mcp2210_gnvram_res.u.usbkeyparams.msb_usb_pid;

				/* Take care to only copy in the power source
				 * bits */
				x = mcp2210_snvram_req.u.usbkeyparams.usb_power_attributes;
				x &= ~(MCP2210_USBPOWER_SELF | MCP2210_USBPOWER_BUSS);
				y = ioctl_put_flash->req.mcp2210_set_req.u.usbkeyparams.usb_power_attributes;
				y &= (MCP2210_USBPOWER_SELF | MCP2210_USBPOWER_BUSS);
				x |= y;
				mcp2210_snvram_req.u.usbkeyparams.usb_power_attributes = x;
				mcp2210_snvram_req.u.usbkeyparams.usb_requested_ma =
				    ioctl_put_flash->req.mcp2210_set_req.u.usbkeyparams.usb_requested_ma;

				/* Do a check to make sure that the SELF and
				 * BUSS powered bits are not both set or both
				 * unset.  You are not suppose to do that
				 * according to the datasheet for the chip. */
				x = mcp2210_snvram_req.u.usbkeyparams.usb_power_attributes &=
				    (MCP2210_USBPOWER_SELF | MCP2210_USBPOWER_BUSS);
				if (x == 0 ||
				    x == (MCP2210_USBPOWER_SELF | MCP2210_USBPOWER_BUSS))
					error = EINVAL;
				break;
			default:
				error = EINVAL;
			};

			if (error)
				break;

			umcpmio_dump_buffer(sc->sc_dumpbuffer,
			    &ioctl_put_flash->req.put_req_blob[0],
			    UMCPMIO_REQ_BUFFER_SIZE,
			    "umcpmio_dev_ioctl: UMCPMIO_PUT_FLASH:"
			    " ioctl put request");
			umcpmio_dump_buffer(sc->sc_dumpbuffer,
			    (uint8_t *) & mcp2210_snvram_req, MCP2210_REQ_BUFFER_SIZE,
			    "umcpmio_dev_ioctl:"
			    " UMCPMIO_PUT_FLASH: mcp2210_snvram_req");


			memset(&mcp2210_snvram_res, 0, MCP2210_RES_BUFFER_SIZE);
			error = mcp2210_set_nvram(sc, &mcp2210_snvram_req,
			    &mcp2210_snvram_res);
			if (error)
				break;

			umcpmio_dump_buffer(sc->sc_dumpbuffer,
			    (uint8_t *) & mcp2210_snvram_res, MCP2210_RES_BUFFER_SIZE,
			    "umcpmio_dev_ioctl: UMCPMIO_PUT_FLASH:"
			    " mcp2210_snvram_res");
			memcpy(&ioctl_put_flash->res.mcp2210_set_res, &mcp2210_snvram_res,
			    MCP2210_RES_BUFFER_SIZE);
		}
		if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2221) {
			/* For the MCP-2221:
			 *
			 * We only allow the flash parts related to gpio to be
			 * changed. Bounce any attempt to do something else.
			 * Also use a shadow buffer for the put, so we get to
			 * control just literally everything about the write to
			 * flash. */
			if (ioctl_put_flash->subcode != MCP2221_FLASH_SUBCODE_GP) {
				error = EINVAL;
				break;
			}
			memset(&mcp2221_put_req, 0, MCP2221_REQ_BUFFER_SIZE);
			mcp2221_put_req.subcode = ioctl_put_flash->subcode;
			mcp2221_put_req.u.gp.gp0_settings =
			    ioctl_put_flash->req.mcp2221_put_req.u.gp.gp0_settings;
			mcp2221_put_req.u.gp.gp1_settings =
			    ioctl_put_flash->req.mcp2221_put_req.u.gp.gp1_settings;
			mcp2221_put_req.u.gp.gp2_settings =
			    ioctl_put_flash->req.mcp2221_put_req.u.gp.gp2_settings;
			mcp2221_put_req.u.gp.gp3_settings =
			    ioctl_put_flash->req.mcp2221_put_req.u.gp.gp3_settings;
			umcpmio_dump_buffer(sc->sc_dumpbuffer,
			    &ioctl_put_flash->req.put_req_blob[0],
			    UMCPMIO_REQ_BUFFER_SIZE,
			    "umcpmio_dev_ioctl: UMCPMIO_PUT_FLASH:"
			    " ioctl mcp2221_put_req");
			umcpmio_dump_buffer(sc->sc_dumpbuffer,
			    (uint8_t *) & mcp2221_put_req, MCP2221_REQ_BUFFER_SIZE,
			    "umcpmio_dev_ioctl:"
			    " UMCPMIO_PUT_FLASH: mcp2221_put_req");

			memset(&mcp2221_put_res, 0, MCP2221_RES_BUFFER_SIZE);
			error = mcp2221_put_flash(sc, &mcp2221_put_req, &mcp2221_put_res);
			if (error)
				break;

			umcpmio_dump_buffer(sc->sc_dumpbuffer,
			    (uint8_t *) & mcp2221_put_res, MCP2221_RES_BUFFER_SIZE,
			    "umcpmio_dev_ioctl: UMCPMIO_PUT_FLASH:"
			    " mcp2221_put_res");

			memcpy(&ioctl_put_flash->res.mcp2221_put_res, &mcp2221_put_res,
			    MCP2221_RES_BUFFER_SIZE);
		}
		break;

	case UMCPMIO_CHIP_TYPE:
		chip_type = (uint8_t *) data;
		*chip_type = 0;
		switch (sc->sc_chipinfo->usb_id) {
		case USB_PRODUCT_MICROCHIP_MCP2210:
			*chip_type = UMCPMIO_CHIP_TYPE_2210;
			break;
		case USB_PRODUCT_MICROCHIP_MCP2221:
			*chip_type = UMCPMIO_CHIP_TYPE_2221;
			break;
		default:
			break;
		}
		break;

	default:
		error = EINVAL;
	}

	mutex_exit(&sc->sc_action_mutex);

	return error;
}

/* This is for sysctl variables that don't actually change the chip.  */

int
umcpmio_verify_sysctl(SYSCTLFN_ARGS)
{
	int error, t;
	struct sysctlnode node;

	node = *rnode;
	t = *(int *)rnode->sysctl_data;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (t < 0)
		return EINVAL;

	*(int *)rnode->sysctl_data = t;

	return 0;
}

/*
 * sysctl validation for stuff that interacts with the chip needs to
 * happen in a transaction.  The read of the current state and the
 * update to new state can't allow for someone to sneak in between the
 * two.
 *
 * We use text for the values of a lot of these variables so you don't
 * need the datasheet in front of you.  You get to do that with
 * umcpmioctl(8).
 */

static struct umcpmio_sysctl_name mcp2221_vref_names[] = {
	{
		.text = "4.096V",
	},
	{
		.text = "2.048V",
	},
	{
		.text = "1.024V",
	},
	{
		.text = "OFF",
	},
	{
		.text = "VDD",
	}
};

int
mcp2221_verify_dac_sysctl(SYSCTLFN_ARGS)
{
	char buf[MCP2221_VREF_NAME];
	char cbuf[MCP2221_VREF_NAME];
	struct umcpmio_softc *sc;
	struct sysctlnode node;
	int error = 0;
	int vrm;
	size_t i;
	struct mcp2221_get_sram_res sram_res;

	node = *rnode;
	sc = node.sysctl_data;

	mutex_enter(&sc->sc_action_mutex);

	error = mcp2221_get_sram(sc, &sram_res);
	if (error)
		goto out;

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *) & sram_res, MCP2221_RES_BUFFER_SIZE,
	    "umcpmio_verify_dac_sysctl SRAM res buffer");

	if (sram_res.dac_reference_voltage & MCP2221_SRAM_DAC_IS_VRM) {
		vrm = sram_res.dac_reference_voltage &
		    MCP2221_SRAM_DAC_VRM_MASK;
		switch (vrm) {
		case MCP2221_SRAM_DAC_VRM_4096V:
			strncpy(buf, "4.096V", MCP2221_VREF_NAME);
			break;
		case MCP2221_SRAM_DAC_VRM_2048V:
			strncpy(buf, "2.048V", MCP2221_VREF_NAME);
			break;
		case MCP2221_SRAM_DAC_VRM_1024V:
			strncpy(buf, "1.024V", MCP2221_VREF_NAME);
			break;
		case MCP2221_SRAM_DAC_VRM_OFF:
		default:
			strncpy(buf, "OFF", MCP2221_VREF_NAME);
			break;
		}
	} else {
		strncpy(buf, "VDD", MCP2221_VREF_NAME);
	}
	strncpy(cbuf, buf, MCP2221_VREF_NAME);
	node.sysctl_data = buf;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		goto out;

	for (i = 0; i < __arraycount(mcp2221_vref_names); i++) {
		if (strncmp(node.sysctl_data, mcp2221_vref_names[i].text,
		    MCP2221_VREF_NAME) == 0) {
			break;
		}
	}
	if (i == __arraycount(mcp2221_vref_names)) {
		error = EINVAL;
		goto out;
	}
	if (strncmp(cbuf, buf, MCP2221_VREF_NAME) == 0) {
		error = 0;
		goto out;
	}
	DPRINTF(("umcpmio_verify_dac_sysctl: setting DAC vref: %s\n", buf));
	error = mcp2221_set_dac_vref_one(sc, buf);

out:
	mutex_exit(&sc->sc_action_mutex);
	return error;
}

int
mcp2221_verify_adc_sysctl(SYSCTLFN_ARGS)
{
	char buf[MCP2221_VREF_NAME];
	char cbuf[MCP2221_VREF_NAME];
	struct umcpmio_softc *sc;
	struct sysctlnode node;
	int error = 0;
	int vrm;
	size_t i;
	struct mcp2221_get_sram_res sram_res;

	node = *rnode;
	sc = node.sysctl_data;

	mutex_enter(&sc->sc_action_mutex);

	error = mcp2221_get_sram(sc, &sram_res);
	if (error)
		goto out;

	if (sram_res.irq_adc_reference_voltage & MCP2221_SRAM_ADC_IS_VRM) {
		vrm = sram_res.irq_adc_reference_voltage &
		    MCP2221_SRAM_ADC_VRM_MASK;
		switch (vrm) {
		case MCP2221_SRAM_ADC_VRM_4096V:
			strncpy(buf, "4.096V", MCP2221_VREF_NAME);
			break;
		case MCP2221_SRAM_ADC_VRM_2048V:
			strncpy(buf, "2.048V", MCP2221_VREF_NAME);
			break;
		case MCP2221_SRAM_ADC_VRM_1024V:
			strncpy(buf, "1.024V", MCP2221_VREF_NAME);
			break;
		case MCP2221_SRAM_ADC_VRM_OFF:
		default:
			strncpy(buf, "OFF", MCP2221_VREF_NAME);
			break;
		}
	} else {
		strncpy(buf, "VDD", MCP2221_VREF_NAME);
	}
	strncpy(cbuf, buf, MCP2221_VREF_NAME);
	node.sysctl_data = buf;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		goto out;

	for (i = 0; i < __arraycount(mcp2221_vref_names); i++) {
		if (strncmp(node.sysctl_data, mcp2221_vref_names[i].text,
		    MCP2221_VREF_NAME) == 0) {
			break;
		}
	}
	if (i == __arraycount(mcp2221_vref_names)) {
		error = EINVAL;
		goto out;
	}
	if (strncmp(cbuf, buf, MCP2221_VREF_NAME) == 0) {
		error = 0;
		goto out;
	}
	DPRINTF(("umcpmio_verify_adc_sysctl: setting ADC vref: %s\n", buf));
	error = mcp2221_set_adc_vref_one(sc, buf);

out:
	mutex_exit(&sc->sc_action_mutex);
	return error;
}

static struct umcpmio_sysctl_name mcp2221_dc_names[] = {
	{
		.text = "75%",
	},
	{
		.text = "50%",
	},
	{
		.text = "25%",
	},
	{
		.text = "0%",
	}
};

static int
mcp2221_verify_gpioclock_dc_sysctl(SYSCTLFN_ARGS)
{
	char buf[MCP2221_DC_NAME];
	char cbuf[MCP2221_DC_NAME];
	struct umcpmio_softc *sc;
	struct sysctlnode node;
	int error = 0;
	uint8_t duty_cycle;
	size_t i;
	struct mcp2221_get_sram_res sram_res;

	node = *rnode;
	sc = node.sysctl_data;

	mutex_enter(&sc->sc_action_mutex);

	error = mcp2221_get_sram(sc, &sram_res);
	if (error)
		goto out;

	duty_cycle = sram_res.clock_divider & MCP2221_SRAM_GPIO_CLOCK_DC_MASK;
	DPRINTF(("umcpmio_verify_gpioclock_dc_sysctl: current duty cycle:"
	    " %02x\n", duty_cycle));
	switch (duty_cycle) {
	case MCP2221_SRAM_GPIO_CLOCK_DC_75:
		strncpy(buf, "75%", MCP2221_DC_NAME);
		break;
	case MCP2221_SRAM_GPIO_CLOCK_DC_50:
		strncpy(buf, "50%", MCP2221_DC_NAME);
		break;
	case MCP2221_SRAM_GPIO_CLOCK_DC_25:
		strncpy(buf, "25%", MCP2221_DC_NAME);
		break;
	case MCP2221_SRAM_GPIO_CLOCK_DC_0:
	default:
		strncpy(buf, "0%", MCP2221_DC_NAME);
		break;
	}
	strncpy(cbuf, buf, MCP2221_DC_NAME);
	node.sysctl_data = buf;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		goto out;

	for (i = 0; i < __arraycount(mcp2221_dc_names); i++) {
		if (strncmp(node.sysctl_data, mcp2221_dc_names[i].text,
		    MCP2221_DC_NAME) == 0) {
			break;
		}
	}
	if (i == __arraycount(mcp2221_dc_names)) {
		error = EINVAL;
		goto out;
	}
	if (strncmp(cbuf, buf, MCP2221_DC_NAME) == 0) {
		error = 0;
		goto out;
	}
	DPRINTF(("umcpmio_verify_gpioclock_dc_sysctl:"
	    " setting GPIO clock duty cycle: %s\n", buf));
	error = mcp2221_set_gpioclock_dc_one(sc, buf);

out:
	mutex_exit(&sc->sc_action_mutex);
	return error;
}

static struct umcpmio_sysctl_name mcp2221_cd_names[] = {
	{
		.text = "375kHz",
	},
	{
		.text = "750kHz",
	},
	{
		.text = "1.5MHz",
	},
	{
		.text = "3MHz",
	},
	{
		.text = "6MHz",
	},
	{
		.text = "12MHz",
	},
	{
		.text = "24MHz",
	}
};

static int
mcp2221_verify_gpioclock_cd_sysctl(SYSCTLFN_ARGS)
{
	char buf[MCP2221_CD_NAME];
	char cbuf[MCP2221_CD_NAME];
	struct umcpmio_softc *sc;
	struct sysctlnode node;
	int error = 0;
	uint8_t clock_divider;
	size_t i;
	struct mcp2221_get_sram_res sram_res;

	node = *rnode;
	sc = node.sysctl_data;

	mutex_enter(&sc->sc_action_mutex);

	error = mcp2221_get_sram(sc, &sram_res);
	if (error)
		goto out;

	clock_divider = sram_res.clock_divider &
	    MCP2221_SRAM_GPIO_CLOCK_CD_MASK;
	DPRINTF(("umcpmio_verify_gpioclock_cd_sysctl: current clock divider:"
	    " %02x\n", clock_divider));
	switch (clock_divider) {
	case MCP2221_SRAM_GPIO_CLOCK_CD_375KHZ:
		strncpy(buf, "375kHz", MCP2221_CD_NAME);
		break;
	case MCP2221_SRAM_GPIO_CLOCK_CD_750KHZ:
		strncpy(buf, "750kHz", MCP2221_CD_NAME);
		break;
	case MCP2221_SRAM_GPIO_CLOCK_CD_1P5MHZ:
		strncpy(buf, "1.5MHz", MCP2221_CD_NAME);
		break;
	case MCP2221_SRAM_GPIO_CLOCK_CD_3MHZ:
		strncpy(buf, "3MHz", MCP2221_CD_NAME);
		break;
	case MCP2221_SRAM_GPIO_CLOCK_CD_6MHZ:
		strncpy(buf, "6MHz", MCP2221_CD_NAME);
		break;
	case MCP2221_SRAM_GPIO_CLOCK_CD_12MHZ:
		strncpy(buf, "12MHz", MCP2221_CD_NAME);
		break;
	case MCP2221_SRAM_GPIO_CLOCK_CD_24MHZ:
		strncpy(buf, "24MHz", MCP2221_CD_NAME);
		break;
	default:
		strncpy(buf, "12MHz", MCP2221_CD_NAME);
		break;
	}
	strncpy(cbuf, buf, MCP2221_CD_NAME);
	node.sysctl_data = buf;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		goto out;

	for (i = 0; i < __arraycount(mcp2221_cd_names); i++) {
		if (strncmp(node.sysctl_data, mcp2221_cd_names[i].text,
		    MCP2221_CD_NAME) == 0) {
			break;
		}
	}
	if (i == __arraycount(mcp2221_cd_names)) {
		error = EINVAL;
		goto out;
	}
	if (strncmp(cbuf, buf, MCP2221_CD_NAME) == 0) {
		error = 0;
		goto out;
	}
	DPRINTF(("umcpmio_verify_gpioclock_cd_sysctl:"
	    " setting GPIO clock clock divider: %s\n",
	    buf));
	error = mcp2221_set_gpioclock_cd_one(sc, buf);

out:
	mutex_exit(&sc->sc_action_mutex);
	return error;
}

static int
mcp2210_verify_counter_sysctl(SYSCTLFN_ARGS)
{
	struct umcpmio_softc *sc;
	struct sysctlnode node;
	int error = 0;
	int counter;

	node = *rnode;
	sc = node.sysctl_data;

	mutex_enter(&sc->sc_action_mutex);

	error = mcp2210_get_gp6_counter(sc, &counter, MCP2210_COUNTER_RETAIN);
	if (error)
		goto out;

	DPRINTF(("mcp2210_verify_counter_sysctl 1: counter=%d\n", counter));

	node.sysctl_data = &counter;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		goto out;

	DPRINTF(("mcp2210_verify_counter_sysctl 2: counter=%d\n", counter));

	*(int *)rnode->sysctl_data = (int)counter;

out:
	mutex_exit(&sc->sc_action_mutex);
	return error;
}

static int
mcp2210_reset_counter_sysctl(SYSCTLFN_ARGS)
{
	struct umcpmio_softc *sc;
	struct sysctlnode node;
	int error = 0;
	int counter;
	bool reset = false;

	node = *rnode;
	sc = node.sysctl_data;

	node.sysctl_data = &reset;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	DPRINTF(("mcp2210_reset_counter_sysctl: reset=%d\n", reset));

	if (reset) {
		mutex_enter(&sc->sc_action_mutex);
		error = mcp2210_get_gp6_counter(sc, &counter, MCP2210_COUNTER_RESET);
		*(int *)rnode->sysctl_data = false;
		mutex_exit(&sc->sc_action_mutex);
	}
	return error;
}

static int
umcpmio_sysctl_init(struct umcpmio_softc *sc)
{
	int error;
	const struct sysctlnode *cnode;
	int sysctlroot_num, i2c_num = 0, spi_num = 0, adc_dac_num = 0, adc_num = 0, dac_num = 0,
	    gpio_num;

	if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
	    0, CTLTYPE_NODE, device_xname(sc->sc_dev),
	    SYSCTL_DESCR("umcpmio controls"),
	    NULL, 0, NULL, 0,
	    CTL_HW, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	sysctlroot_num = cnode->sysctl_num;

#ifdef UMCPMIO_DEBUG
	if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "debug",
	    SYSCTL_DESCR("Debug level"),
	    umcpmio_verify_sysctl, 0, &umcpmiodebug, 0,
	    CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "dump_buffers",
	    SYSCTL_DESCR("Dump buffer when debugging"),
	    NULL, 0, &sc->sc_dumpbuffer, 0,
	    CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;
#endif

	if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "response_wait",
	    SYSCTL_DESCR("How long to wait in ms for a response"
	    " for a HID report"),
	    umcpmio_verify_sysctl, 0, &sc->sc_cv_wait, 0,
	    CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "response_errcnt",
	    SYSCTL_DESCR("How many errors to allow on a response"),
	    umcpmio_verify_sysctl, 0, &sc->sc_response_errcnt, 0,
	    CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2210) {
		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    0, CTLTYPE_NODE, "spi",
		    SYSCTL_DESCR("SPI controls"),
		    NULL, 0, NULL, 0,
		    CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;

		spi_num = cnode->sysctl_num;
	}
	if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2221) {
		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    0, CTLTYPE_NODE, "i2c",
		    SYSCTL_DESCR("I2C controls"),
		    NULL, 0, NULL, 0,
		    CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;

		i2c_num = cnode->sysctl_num;

		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    0, CTLTYPE_NODE, "adcdac",
		    SYSCTL_DESCR("ADC and DAC controls"),
		    NULL, 0, NULL, 0,
		    CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;

		adc_dac_num = cnode->sysctl_num;

		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    0, CTLTYPE_NODE, "adc",
		    SYSCTL_DESCR("ADC controls"),
		    NULL, 0, NULL, 0,
		    CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;

		adc_num = cnode->sysctl_num;

		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    0, CTLTYPE_NODE, "dac",
		    SYSCTL_DESCR("DAC controls"),
		    NULL, 0, NULL, 0,
		    CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;

		dac_num = cnode->sysctl_num;
	}
	if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
	    0, CTLTYPE_NODE, "gpio",
	    SYSCTL_DESCR("GPIO controls"),
	    NULL, 0, NULL, 0,
	    CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	gpio_num = cnode->sysctl_num;

	if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2210) {
		/* SPI */
		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_BOOL, "verbose",
		    SYSCTL_DESCR("Verbose SPI messages"),
		    umcpmio_verify_sysctl, 0, &sc->sc_spi_verbose, 0,
		    CTL_HW, sysctlroot_num, spi_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;

		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_INT, "busy_delay",
		    SYSCTL_DESCR("How long to wait in ms when the SPI engine is busy"),
		    umcpmio_verify_sysctl, 0, &sc->sc_busy_delay, 0,
		    CTL_HW, sysctlroot_num, spi_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;

		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_INT, "retry_busy_chipdrain",
		    SYSCTL_DESCR("How many times to retry the SPI engine to drain the chip"),
		    umcpmio_verify_sysctl, 0, &sc->sc_retry_busy_read, 0,
		    CTL_HW, sysctlroot_num, spi_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;

		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_INT, "retry_busy_transfer",
		    SYSCTL_DESCR("How many times to retry the SPI engine in a normal transfer"),
		    umcpmio_verify_sysctl, 0, &sc->sc_retry_busy_write, 0,
		    CTL_HW, sysctlroot_num, spi_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;
	}
	if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2221) {
		/* I2C */
		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_BOOL, "reportreadnostop",
		    SYSCTL_DESCR("Report that a READ without STOP was attempted"
		    " by a device"),
		    NULL, 0, &sc->sc_reportreadnostop, 0,
		    CTL_HW, sysctlroot_num, i2c_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;

		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_INT, "busy_delay",
		    SYSCTL_DESCR("How long to wait in ms when the I2C engine is busy"),
		    umcpmio_verify_sysctl, 0, &sc->sc_busy_delay, 0,
		    CTL_HW, sysctlroot_num, i2c_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;

		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_INT, "retry_busy_read",
		    SYSCTL_DESCR("How many times to retry a busy I2C read"),
		    umcpmio_verify_sysctl, 0, &sc->sc_retry_busy_read, 0,
		    CTL_HW, sysctlroot_num, i2c_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;

		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_INT, "retry_busy_write",
		    SYSCTL_DESCR("How many times to retry a busy I2C write"),
		    umcpmio_verify_sysctl, 0, &sc->sc_retry_busy_write, 0,
		    CTL_HW, sysctlroot_num, i2c_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;
	}

	/* GPIO */
	if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2210) {
		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    CTLFLAG_READONLY, CTLTYPE_INT, "counter",
		    SYSCTL_DESCR("Interrupt counter on GP6"),
		    mcp2210_verify_counter_sysctl, 0, (void *)sc, 0,
		    CTL_HW, sysctlroot_num, gpio_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;

		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_BOOL, "reset_counter",
		    SYSCTL_DESCR("Reset interrupt counter on GP6"),
		    mcp2210_reset_counter_sysctl, 0, (void *)sc, 0,
		    CTL_HW, sysctlroot_num, gpio_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;
	}
	if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2221) {
		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    CTLFLAG_READONLY, CTLTYPE_STRING, "clock_duty_cycles",
		    SYSCTL_DESCR("Valid duty cycles for GPIO clock on"
		    " GP1 ALT3 duty cycle"),
		    0, 0, __UNCONST(umcpmio_valid_dcs), sizeof(umcpmio_valid_dcs) + 1,
		    CTL_HW, sysctlroot_num, gpio_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;

		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_STRING, "clock_duty_cycle",
		    SYSCTL_DESCR("GPIO clock on GP1 ALT3 duty cycle"),
		    mcp2221_verify_gpioclock_dc_sysctl, 0, (void *)sc, MCP2221_DC_NAME,
		    CTL_HW, sysctlroot_num, gpio_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;

		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    CTLFLAG_READONLY, CTLTYPE_STRING, "clock_dividers",
		    SYSCTL_DESCR("Valid clock dividers for GPIO clock on GP1"
		    " with ALT3"),
		    0, 0, __UNCONST(umcpmio_valid_cds), sizeof(umcpmio_valid_cds) + 1,
		    CTL_HW, sysctlroot_num, gpio_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;

		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_STRING, "clock_divider",
		    SYSCTL_DESCR("GPIO clock on GP1 ALT3 clock divider"),
		    mcp2221_verify_gpioclock_cd_sysctl, 0, (void *)sc, MCP2221_CD_NAME,
		    CTL_HW, sysctlroot_num, gpio_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;

		/* ADC and DAC */
		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    CTLFLAG_READONLY, CTLTYPE_STRING, "vrefs",
		    SYSCTL_DESCR("Valid vref values for ADC and DAC"),
		    0, 0,
		    __UNCONST(umcpmio_valid_vrefs), sizeof(umcpmio_valid_vrefs) + 1,
		    CTL_HW, sysctlroot_num, adc_dac_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;

		/* ADC */
		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_STRING, "vref",
		    SYSCTL_DESCR("ADC voltage reference"),
		    mcp2221_verify_adc_sysctl, 0, (void *)sc, MCP2221_VREF_NAME,
		    CTL_HW, sysctlroot_num, adc_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;

		/* DAC */
		if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_STRING, "vref",
		    SYSCTL_DESCR("DAC voltage reference"),
		    mcp2221_verify_dac_sysctl, 0, (void *)sc, MCP2221_VREF_NAME,
		    CTL_HW, sysctlroot_num, dac_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;
	}
	return 0;
}

static int
umcpmio_match(device_t parent, cfdata_t match, void *aux)
{
	struct uhidev_attach_arg *uha = aux;

	return umcpmio_lookup(uha->uiaa->uiaa_vendor, uha->uiaa->uiaa_product)
	    != NULL ? UMATCH_VENDOR_PRODUCT : UMATCH_NONE;
}

static void
umcpmio_attach(device_t parent, device_t self, void *aux)
{
	struct umcpmio_softc *sc = device_private(self);
	struct uhidev_attach_arg *uha = aux;
	struct mcp2221_status_res status_res;
	int err;

	sc->sc_dev = self;
	sc->sc_hdev = uha->parent;
	sc->sc_udev = uha->uiaa->uiaa_device;

	sc->sc_umcpmiolog = NULL;
	sc->sc_dumpbuffer = false;

	sc->sc_cv_wait = 2500;
	sc->sc_response_errcnt = 5;
	sc->sc_dev_open[CONTROL_DEV] = false;
	sc->sc_dev_open[GP1_DEV] = false;
	sc->sc_dev_open[GP2_DEV] = false;
	sc->sc_dev_open[GP3_DEV] = false;
	sc->sc_dev_open[EEPROM_DEV] = false;
	sc->sc_adcdac_pin_flags[0] =
	    sc->sc_adcdac_pin_flags[1] =
	    sc->sc_adcdac_pin_flags[2] =
	    sc->sc_adcdac_pin_flags[3] = -1;

	aprint_normal("\n");

	int info_index = -1;
	for (int c = 0; c < __arraycount(umcpmio_chip_infos); c++){
		if (uha->uiaa->uiaa_product == umcpmio_chip_infos[c].usb_id) {
			info_index = c;
			break;
		}
	}

	if (info_index == -1)
		panic("umcpmio_attach: Unknown info_index\n");

	sc->sc_chipinfo = &umcpmio_chip_infos[info_index];

	if ((err = umcpmio_sysctl_init(sc)) != 0) {
		aprint_error_dev(self, "Can't setup sysctl tree (%d)\n", err);
		return;
	}
	mutex_init(&sc->sc_action_mutex, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->sc_res_cv, "mcpres");
	mutex_init(&sc->sc_res_mutex, MUTEX_DEFAULT, IPL_NONE);
	sc->sc_res_buffer = NULL;
	sc->sc_res_ready = false;
	mutex_init(&sc->sc_spi_mutex, MUTEX_DEFAULT, IPL_NONE);

	err = umcpmio_hid_open(sc);
	if (err)
		return;

	if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2210) {
		aprint_normal_dev(sc->sc_dev, "MCP-2210\n");
	}
	if (sc->sc_chipinfo->usb_id == USB_PRODUCT_MICROCHIP_MCP2221) {
		mutex_enter(&sc->sc_action_mutex);
		err = mcp2221_get_status(sc, &status_res);
		mutex_exit(&sc->sc_action_mutex);
		if (err) {
			aprint_error_dev(sc->sc_dev, "umcpmio_attach: "
			    " mcp2221_get_status: err=%d\n", err);
			return;
		}
		aprint_normal_dev(sc->sc_dev,
		    "MCP-2221 / MCP-2221A: "
		    "Hardware revision: %d.%d, Firmware revision: %d.%d\n",
		    status_res.mcp2221_hardware_rev_major,
		    status_res.mcp2221_hardware_rev_minor,
		    status_res.mcp2221_firmware_rev_major,
		    status_res.mcp2221_firmware_rev_minor);

	}

	/* The IIC or SPI bus should attach, but if they will not, do not
	 * get too upset about it.
	 */

	if (sc->sc_chipinfo->num_spi_slaves > 0) {
		err = umcpmio_spi_attach(sc);
		if (err)
			aprint_error_dev(sc->sc_dev, "umcpmio_attach: Unable"
			    " to attach SPI bus.  Continuing anyway.\n");
	}
	if (sc->sc_chipinfo->num_iic_ports > 0) {
		err = umcpmio_i2c_attach(sc);
		if (err)
			aprint_error_dev(sc->sc_dev, "umcpmio_attach: Unable"
			    " to set I2C speed.  Continuing anyway.\n");
	}
	if (sc->sc_chipinfo->num_gpio_pins > 0)
		umcpmio_gpio_attach(sc);
}

static int
umcpmio_detach(device_t self, int flags)
{
	struct umcpmio_softc *sc = device_private(self);
	struct umcpmio_spi_received *r;
	int err;

	sc->sc_dying = true;

	/* Not sure if this is needed.... */

	flags |= DETACH_FORCE;

	err = config_detach_children(self, flags);
	if (err)
		return err;

	mutex_enter(&sc->sc_action_mutex);
	uhidev_close(sc->sc_hdev);
	mutex_destroy(&sc->sc_res_mutex);
	cv_destroy(&sc->sc_res_cv);
	sysctl_teardown(&sc->sc_umcpmiolog);
	mutex_exit(&sc->sc_action_mutex);
	mutex_destroy(&sc->sc_action_mutex);

	if (sc->sc_chipinfo->num_spi_slaves > 0) {
		mutex_enter(&sc->sc_spi_mutex);
		for (int slave = 0; slave < sc->sc_chipinfo->num_spi_slaves; slave++){
			if (!SIMPLEQ_EMPTY(&sc->sc_received[slave])) {
				while ((r = SIMPLEQ_FIRST(&sc->sc_received[slave])) != NULL) {
					SIMPLEQ_REMOVE_HEAD(&sc->sc_received[slave], umcpmio_spi_received_q);
					kmem_free(r, sizeof(struct umcpmio_spi_received));
				}
			}
		}
		mutex_exit(&sc->sc_spi_mutex);
	}
	mutex_destroy(&sc->sc_spi_mutex);

	return 0;
}

static int
umcpmio_activate(device_t self, enum devact act)
{
	struct umcpmio_softc *sc = device_private(self);

	DPRINTFN(5, ("umcpmio_activate: %d\n", act));

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = true;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

/*	$NetBSD: umcpmio.c,v 1.3 2025/03/25 20:38:27 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: umcpmio.c,v 1.3 2025/03/25 20:38:27 riastradh Exp $");

/*
 * Driver for the Microchip MCP2221 / MCP2221A USB multi-io chip
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

#include <dev/i2c/i2cvar.h>

#include <dev/gpio/gpiovar.h>

#include <dev/hid/hid.h>

#include <dev/usb/uhidev.h>
#include <dev/usb/usb.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbhid.h>

#include <dev/usb/umcpmio.h>
#include <dev/usb/umcpmio_hid_reports.h>
#include <dev/usb/umcpmio_io.h>
#include <dev/usb/umcpmio_subr.h>

int umcpmio_send_report(struct umcpmio_softc *, uint8_t *, size_t, uint8_t *,
    int);

static const struct usb_devno umcpmio_devs[] = {
	{ USB_VENDOR_MICROCHIP, USB_PRODUCT_MICROCHIP_MCP2221 },
};
#define umcpmio_lookup(v, p) usb_lookup(umcpmio_devs, v, p)

static int	umcpmio_match(device_t, cfdata_t, void *);
static void	umcpmio_attach(device_t, device_t, void *);
static int	umcpmio_detach(device_t, int);
static int	umcpmio_activate(device_t, enum devact);
static int 	umcpmio_verify_sysctl(SYSCTLFN_ARGS);
static int	umcpmio_verify_dac_sysctl(SYSCTLFN_ARGS);
static int	umcpmio_verify_adc_sysctl(SYSCTLFN_ARGS);
static int	umcpmio_verify_gpioclock_dc_sysctl(SYSCTLFN_ARGS);
static int	umcpmio_verify_gpioclock_cd_sysctl(SYSCTLFN_ARGS);

#define UMCPMIO_DEBUG 1
#ifdef UMCPMIO_DEBUG
#define DPRINTF(x)	do { if (umcpmiodebug) printf x; } while (0)
#define DPRINTFN(n, x)	do { if (umcpmiodebug > (n)) printf x; } while (0)
int	umcpmiodebug = 0;
#else
#define DPRINTF(x)	__nothing
#define DPRINTFN(n, x)	__nothing
#endif

CFATTACH_DECL_NEW(umcpmio, sizeof(struct umcpmio_softc), umcpmio_match,
    umcpmio_attach, umcpmio_detach, umcpmio_activate);

static void
WAITMS(int ms)
{
	if (ms > 0)
		delay(ms * 1000);
}

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

static void
umcpmio_dump_buffer(bool enabled, uint8_t *buf, u_int len, const char *name)
{
	int i;

	if (enabled) {
		DPRINTF(("%s:", name));
		for (i = 0; i < len; i++) {
			DPRINTF((" %02x", buf[i]));
		}
		DPRINTF(("\n"));
	}
}

/*
 * Communication with the HID function requires sending a HID report
 * request and then waiting for a response.
 *
 * The panic that occurs when trying to use the interrupt... i.e.
 * attaching though this driver seems to be related to the fact that
 * a spin lock is held and the USB stack wants to wait.
 *
 * The USB stack *IS* going to have to wait for the response from
 * the device, somehow...
 *
 * It didn't seem possible to defer the uhidev_write to a thread.
 * Attempts to yield() while spinning hard also did not work and
 * not yield()ing didn't allow anything else to run.
 *
 */

/*
 * This is the panic you will get:
 *
 * panic: kernel diagnostic assertion "ci->ci_mtx_count == -1" failed: file "../../../../kern/kern_synch.c", line 762 mi_switch: cpu0: ci_mtx_count (-2) != -1 (block with spin-mutex held)
 */

static void
umcpmio_uhidev_intr(void *cookie, void *ibuf, u_int len)
{
	struct umcpmio_softc *sc = cookie;

	if (sc->sc_dying)
		return;

	DPRINTFN(30, ("umcpmio_uhidev_intr: len=%d\n", len));

	mutex_enter(&sc->sc_res_mutex);
	switch(len) {
	case MCP2221_RES_BUFFER_SIZE:
		if (sc->sc_res_buffer != NULL) {
			memcpy(sc->sc_res_buffer, ibuf,
			    MCP2221_RES_BUFFER_SIZE);
			sc->sc_res_ready = true;
			cv_signal(&sc->sc_res_cv);
		} else {
			int d = umcpmiodebug;
			device_printf(sc->sc_dev,
			    "umcpmio_uhidev_intr: NULL sc_res_buffer:"
			    " len=%d\n",
			    len);
			umcpmiodebug = 20;
			umcpmio_dump_buffer(true, (uint8_t *)ibuf, len,
			    "umcpmio_uhidev_intr: ibuf");
			umcpmiodebug = d;
		}

		break;
	default:
		device_printf(sc->sc_dev,
		    "umcpmio_uhidev_intr: Unknown interrupt length: %d",
		    len);
		break;
	}
	mutex_exit(&sc->sc_res_mutex);
}

/* Send a HID report.  This needs to be called with the action mutex held */

int
umcpmio_send_report(struct umcpmio_softc *sc, uint8_t *sendbuf,
    size_t sendlen, uint8_t *resbuf, int timeout)
{
	int err = 0;
	int err_count = 0;

	if (sc->sc_dying)
		return EIO;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	if (sc->sc_res_buffer != NULL) {
		device_printf(sc->sc_dev,
		    "umcpmio_send_report: sc->sc_res_buffer is not NULL\n");
	}
	sc->sc_res_buffer = resbuf;
	sc->sc_res_ready = false;

	err = uhidev_write(sc->sc_hdev, sendbuf, sendlen);

	if (err) {
		DPRINTF(("umcpmio_send_report: uhidev_write errored with:"
			" err=%d\n", err));
		goto out;
	}

	DPRINTFN(30, ("umcpmio_send_report: about to wait on cv.  err=%d\n",
		err));

	mutex_enter(&sc->sc_res_mutex);
	while (!sc->sc_res_ready) {
		DPRINTFN(20, ("umcpmio_send_report: LOOP for response."
			"  sc_res_ready=%d, err_count=%d, timeout=%d\n",
			sc->sc_res_ready, err_count, mstohz(timeout)));

		err = cv_timedwait_sig(&sc->sc_res_cv, &sc->sc_res_mutex,
		    mstohz(timeout));

		/*
		 * We are only going to allow this to loop on an error,
		 * any error at all, so many times.
		 */
		if (err) {
			DPRINTF(("umcpmio_send_report:"
				" cv_timedwait_sig reported an error:"
				" err=%d, sc->sc_res_ready=%d\n",
				err, sc->sc_res_ready));
			err_count++;
		}

		/*
		 * The CV was interrupted, but the buffer is ready so,
		 * clear the error and break out.
		 */
		if ((err == ERESTART) && (sc->sc_res_ready)) {
			DPRINTF(("umcpmio_send_report:"
				" ERESTART and buffer is ready\n"));
			err = 0;
			break;
		}

		/*
		 * Too many times though the loop, just break out.  Turn
		 * a ERESTART (interruption) into a I/O error at this point.
		 */
		if (err_count > sc->sc_response_errcnt) {
			DPRINTF(("umcpmio_send_report: err_count exceeded:"
				" err=%d\n", err));
			if (err == ERESTART)
				err = EIO;
			break;
		}

		/* This is a normal timeout, without interruption, try again */
		if (err == EWOULDBLOCK) {
			DPRINTF(("umcpmio_send_report: EWOULDBLOCK:"
				" err_count=%d\n", err_count));
			continue;
		}

		/*
		 * The CV was interrupted and the buffer wasn't filled
		 * in, so try again
		 */
		if ((err == ERESTART) && (!sc->sc_res_ready)) {
			DPRINTF(("umcpmio_send_report:"
				" ERESTART and buffer is NOT ready."
				"  err_count=%d\n", err_count));
			continue;
		}
	}

	sc->sc_res_buffer = NULL;
	sc->sc_res_ready = false;
	mutex_exit(&sc->sc_res_mutex);

	/* Turn most errors into an I/O error */
	if (err &&
	    err != ERESTART)
		err = EIO;

 out:
	return err;
}

/* These are standard gpio reads and set calls */

static int
umcpmio_gpio_pin_read(void *arg, int pin)
{
	struct umcpmio_softc *sc = arg;
	int r = GPIO_PIN_LOW;

	r = umcpmio_get_gpio_value(sc, pin, true);

	return r;
}

static void
umcpmio_gpio_pin_write(void *arg, int pin, int value)
{
	struct umcpmio_softc *sc = arg;

	umcpmio_set_gpio_value_one(sc, pin, value, true);
}

/*
 * Internal function that does the dirty work of setting a gpio
 * pin to its "type".
 *
 * There are really two ways to do some of this, one is to set the pin
 * to input and output, or whatever, using SRAM calls, the other is to
 * use the GPIO config calls to set input and output and SRAM for
 * everything else.  This just uses SRAM for everything.
 */

static int
umcpmio_gpio_pin_ctlctl(void *arg, int pin, int flags, bool takemutex)
{
	struct umcpmio_softc *sc = arg;
	struct mcp2221_set_sram_req set_sram_req;
	struct mcp2221_set_sram_res set_sram_res;
	struct mcp2221_get_sram_res current_sram_res;
	struct mcp2221_get_gpio_cfg_res current_gpio_cfg_res;
	int err = 0;

	if (sc->sc_dying)
		return 0;

	if (takemutex)
		mutex_enter(&sc->sc_action_mutex);

	err = umcpmio_get_sram(sc, &current_sram_res, false);
	if (err)
		goto out;

	err = umcpmio_get_gpio_cfg(sc, &current_gpio_cfg_res, false);
	if (err)
		goto out;

	/*
	 * You can't just set one pin, you must set all of them, so copy the
	 * current settings for the pin we are not messing with.
	 *
	 * And, yes, of course, if the MCP-2210 is ever supported with this
	 * driver, this sort of unrolling will need to be turned into
	 * something different, but for now, just unroll as there are only
	 * 4 pins to care about.
	 *
	 */

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
	umcpmio_set_gpio_designation_sram(&set_sram_req, pin, flags);
	umcpmio_set_gpio_dir_sram(&set_sram_req, pin, flags);

	/*
	* This part is unfortunate...  if a pin is set to output, the
	* value set on the pin is not mirrored by the chip into SRAM,
	* but the chip will use the value from SRAM to set the value of
	* the pin.  What this means is that we have to learn the value
	* from the GPIO config and make sure it is set properly when
	* updating SRAM.
	*/

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

	err = umcpmio_put_sram(sc, &set_sram_req, &set_sram_res, false);
	if (err)
		goto out;

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *)&set_sram_res, MCP2221_RES_BUFFER_SIZE,
	    "umcpmio_gpio_pin_ctlctl set sram buffer copy");
	if (set_sram_res.cmd != MCP2221_CMD_SET_SRAM ||
	    set_sram_res.completion != MCP2221_CMD_COMPLETE_OK) {
		device_printf(sc->sc_dev, "umcpmio_gpio_pin_ctlctl:"
		    " not the command desired, or error: %02x %02x\n",
		    set_sram_res.cmd,
		    set_sram_res.completion);
		err = EIO;
		goto out;
	}

	sc->sc_gpio_pins[pin].pin_flags = flags;
	err = 0;

 out:
	if (takemutex)
		mutex_exit(&sc->sc_action_mutex);

	return err;
}

static void
umcpmio_gpio_pin_ctl(void *arg, int pin, int flags)
{
	struct umcpmio_softc *sc = arg;

	if (sc->sc_dying)
		return;

	umcpmio_gpio_pin_ctlctl(sc, pin, flags, true);
}

/*
 * XXX -
 *
 * Since testing of gpio interrupts wasn't possible, this part probably
 * is not complete.  At the very least, there is a scheduled callout
 * that needs to exist to read the interrupt status.  The chip does not
 * send anything on its own when the interrupt happens.
 */

static void *
umcpmio_gpio_intr_establish(void *vsc, int pin, int ipl, int irqmode,
    int (*func)(void *), void *arg)
{
	struct umcpmio_softc *sc = vsc;
	struct umcpmio_irq *irq = &sc->sc_gpio_irqs[0];
	struct mcp2221_set_sram_req set_sram_req;
	struct mcp2221_set_sram_res set_sram_res;
	struct mcp2221_get_sram_res current_sram_res;
	int err = 0;

	if (sc->sc_dying)
		return NULL;

	irq->sc_gpio_irqfunc = func;
	irq->sc_gpio_irqarg = arg;

	DPRINTF(("umcpmio_intr_establish: pin=%d, irqmode=%04x\n",
		pin, irqmode));

	mutex_enter(&sc->sc_action_mutex);

	err = umcpmio_get_sram(sc, &current_sram_res, false);
	if (err)
		goto out;

	memset(&set_sram_req, 0, MCP2221_REQ_BUFFER_SIZE);
	set_sram_req.gp0_settings = current_sram_res.gp0_settings;
	set_sram_req.gp2_settings = current_sram_res.gp2_settings;
	set_sram_req.gp3_settings = current_sram_res.gp3_settings;
	umcpmio_set_gpio_irq_sram(&set_sram_req, irqmode);
	err = umcpmio_put_sram(sc, &set_sram_req, &set_sram_res, false);
	if (err) {
		device_printf(sc->sc_dev, "umcpmio_intr_establish:"
		    " set sram error: err=%d\n",
		    err);
		goto out;
	}
	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *)&set_sram_res, MCP2221_RES_BUFFER_SIZE,
	    "umcpmio_intr_establish set sram buffer copy");
	if (set_sram_res.cmd != MCP2221_CMD_SET_SRAM ||
	    set_sram_res.completion != MCP2221_CMD_COMPLETE_OK) {
		device_printf(sc->sc_dev, "umcpmio_intr_establish:"
		    " not the command desired, or error: %02x %02x\n",
		    set_sram_res.cmd,
		    set_sram_res.completion);
		goto out;
	}

	sc->sc_gpio_pins[1].pin_flags = GPIO_PIN_ALT2;

 out:
	mutex_exit(&sc->sc_action_mutex);

	return irq;
}

static void
umcpmio_gpio_intr_disestablish(void *vsc, void *ih)
{
	struct umcpmio_softc *sc = vsc;
	struct mcp2221_set_sram_req set_sram_req;
	struct mcp2221_set_sram_res set_sram_res;
	struct mcp2221_get_sram_res current_sram_res;
	int err = 0;

	if (sc->sc_dying)
		return;

	DPRINTF(("umcpmio_intr_disestablish:\n"));

	mutex_enter(&sc->sc_action_mutex);

	err = umcpmio_get_sram(sc, &current_sram_res, false);
	if (err)
		goto out;

	memset(&set_sram_req, 0, MCP2221_REQ_BUFFER_SIZE);
	set_sram_req.gp0_settings = current_sram_res.gp0_settings;
	set_sram_req.gp2_settings = current_sram_res.gp2_settings;
	set_sram_req.gp3_settings = current_sram_res.gp3_settings;
	umcpmio_set_gpio_irq_sram(&set_sram_req, 0);
	err = umcpmio_put_sram(sc, &set_sram_req, &set_sram_res, true);
	if (err) {
		device_printf(sc->sc_dev, "umcpmio_intr_disestablish:"
		    " set sram error: err=%d\n",
		    err);
		goto out;
	}

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *)&set_sram_res, MCP2221_RES_BUFFER_SIZE,
	    "umcpmio_intr_disestablish set sram buffer copy");
	if (set_sram_res.cmd == MCP2221_CMD_SET_SRAM &&
	    set_sram_res.completion == MCP2221_CMD_COMPLETE_OK) {
		device_printf(sc->sc_dev, "umcpmio_intr_disestablish:"
		    " not the command desired, or error: %02x %02x\n",
		    set_sram_res.cmd,
		    set_sram_res.completion);
		goto out;
	}

	sc->sc_gpio_pins[1].pin_flags = GPIO_PIN_INPUT;

 out:
	mutex_exit(&sc->sc_action_mutex);
}

static bool
umcpmio_gpio_intrstr(void *vsc, int pin, int irqmode, char *buf, size_t buflen)
{

        if (pin < 0 || pin >= MCP2221_NPINS) {
		DPRINTF(("umcpmio_gpio_intrstr:"
			" pin %d less than zero or too big\n",
			pin));
                return false;
	}

	if (pin != 1) {
		DPRINTF(("umcpmio_gpio_intrstr: pin %d was not 1\n",
			pin));
		return false;
	}

        snprintf(buf, buflen, "GPIO %d", pin);

        return true;
}

/* Clear status of the I2C engine */

static int
umcpmio_i2c_clear(struct umcpmio_softc *sc, bool takemutex)
{
	int err = 0;
	struct mcp2221_status_req status_req;
	struct mcp2221_status_res status_res;

	memset(&status_req, 0, MCP2221_REQ_BUFFER_SIZE);
	status_req.cmd = MCP2221_CMD_STATUS;
	status_req.cancel_transfer = MCP2221_I2C_DO_CANCEL;

	if (takemutex)
		mutex_enter(&sc->sc_action_mutex);
	err = umcpmio_send_report(sc,
	    (uint8_t *)&status_req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *)&status_res, sc->sc_cv_wait);
	if (takemutex)
		mutex_exit(&sc->sc_action_mutex);
	if (err) {
		device_printf(sc->sc_dev, "umcpmio_i2c_clear: request error:"
		    " err=%d\n", err);
		err = EIO;
		goto out;
	}

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *)&status_res, MCP2221_RES_BUFFER_SIZE,
	    "umcpmio_i2c_clear buffer copy");

	if (status_res.cmd != MCP2221_CMD_STATUS &&
	    status_res.completion != MCP2221_CMD_COMPLETE_OK) {
		device_printf(sc->sc_dev, "umcpmio_i2c_clear:"
		    " cmd exec: not the command desired, or error:"
		    " %02x %02x\n",
		    status_res.cmd,
		    status_res.completion);
		err = EIO;
		goto out;
	}

	umcpmio_dump_buffer(true,
	    (uint8_t *)&status_res, MCP2221_RES_BUFFER_SIZE,
	    "umcpmio_i2c_clear res buffer");

out:
	return err;
}

/*
 * There isn't much required to acquire or release the I2C bus, but the man
 * pages says these are needed
 */

static int
umcpmio_acquire_bus(void *v, int flags)
{
	return 0;
}

static void
umcpmio_release_bus(void *v, int flags)
{
	return;
}

/*
 * The I2C write and I2C read functions mostly use an algorithm that Adafruit
 * came up with in their Python based driver.  A lot of other people have used
 * this same algorithm to good effect.  If changes are made to the I2C read and
 * write functions, it is HIGHLY advisable that a MCP2221 or MCP2221A be on
 * hand to test them.
 */

/* This is what is considered a fatal return from the engine. */

static bool
umcpmio_i2c_fatal(uint8_t state)
{
	int r = false;

	if (state == MCP2221_ENGINE_ADDRNACK ||
	    state == MCP2221_ENGINE_STARTTIMEOUT ||
	    state == MCP2221_ENGINE_REPSTARTTIMEOUT ||
	    state == MCP2221_ENGINE_STOPTIMEOUT ||
	    state == MCP2221_ENGINE_READTIMEOUT ||
	    state == MCP2221_ENGINE_WRITETIMEOUT ||
	    state == MCP2221_ENGINE_ADDRTIMEOUT)
		r = true;
	return r;
}

static int
umcpmio_i2c_write(struct umcpmio_softc *sc, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *databuf, size_t datalen,
    int flags)
{
	struct mcp2221_i2c_req i2c_req;
	struct mcp2221_i2c_res i2c_res;
	struct mcp2221_status_res status_res;
	int remaining;
	int err = 0;
	uint8_t cmd;
	size_t totallen = 0;
	int wretry = sc->sc_retry_busy_write;
	int wsretry = sc->sc_retry_busy_write;

	err = umcpmio_get_status(sc, &status_res, true);
	if (err)
		goto out;
	if (status_res.internal_i2c_state != 0) {
		DPRINTF(("umcpmio_i2c_write: internal state not zero,"
			" clearing. internal_i2c_state=%02x\n",
			status_res.internal_i2c_state));
		err = umcpmio_i2c_clear(sc, true);
	}
	if (err)
		goto out;

	if (cmdbuf != NULL)
		totallen += cmdlen;
	if (databuf != NULL)
		totallen += datalen;

 again:
	memset(&i2c_req, 0, MCP2221_REQ_BUFFER_SIZE);
	cmd = MCP2221_I2C_WRITE_DATA_NS;
	if (I2C_OP_STOP_P(op))
		cmd = MCP2221_I2C_WRITE_DATA;
	i2c_req.cmd = cmd;
	i2c_req.lsblen = totallen;
	i2c_req.msblen = 0;
	i2c_req.slaveaddr = addr << 1;

	remaining = 0;
	if (cmdbuf != NULL) {
		memcpy(&i2c_req.data[0], cmdbuf, cmdlen);
		remaining = cmdlen;
	}
	if (databuf != NULL)
		memcpy(&i2c_req.data[remaining], databuf, datalen);

	DPRINTF(("umcpmio_i2c_write: I2C WRITE: cmd: %02x\n", cmd));
	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *)&i2c_req, MCP2221_REQ_BUFFER_SIZE,
	    "umcpmio_i2c_write: write req buffer copy");

	mutex_enter(&sc->sc_action_mutex);
	err = umcpmio_send_report(sc,
	    (uint8_t *)&i2c_req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *)&i2c_res, sc->sc_cv_wait);
	mutex_exit(&sc->sc_action_mutex);
	if (err) {
		device_printf(sc->sc_dev, "umcpmio_i2c_write request error:"
		    " err=%d\n", err);
		err = EIO;
		goto out;
	}

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *)&i2c_res, MCP2221_RES_BUFFER_SIZE,
	    "umcpmio_i2c_write: write res buffer copy");
	if (i2c_res.cmd == cmd &&
	    i2c_res.completion == MCP2221_CMD_COMPLETE_OK) {
		/*
		 * Adafruit does a read back of the status at
		 * this point.  We choose not to do that.  That
		 * is done later anyway, and it seemed to be
		 * redundent.
		 */
	} else if (i2c_res.cmd == cmd &&
	    i2c_res.completion == MCP2221_I2C_ENGINE_BUSY) {
		DPRINTF(("umcpmio_i2c_write:"
			" I2C engine busy\n"));

		if (umcpmio_i2c_fatal(i2c_res.internal_i2c_state)) {
			err = EIO;
			goto out;
		}
		wretry--;
		if (wretry > 0) {
			WAITMS(sc->sc_busy_delay);
			goto again;
		} else {
			err = EBUSY;
			goto out;
		}
	} else {
		device_printf(sc->sc_dev, "umcpmio_i2c_write:"
		    "  not the command desired, or error:"
		    " %02x %02x\n",
		    i2c_res.cmd,
		    i2c_res.completion);
		err = EIO;
		goto out;
	}

	while (wsretry > 0) {
		wsretry--;

		DPRINTF(("umcpmio_i2c_write: checking status loop:"
			" wcretry=%d\n", wsretry));

		err = umcpmio_get_status(sc, &status_res, true);
		if (err) {
			err = EIO;
			break;
		}
		umcpmio_dump_buffer(sc->sc_dumpbuffer,
		    (uint8_t *)&status_res,
		    MCP2221_RES_BUFFER_SIZE,
		    "umcpmio_i2c_write post check status");
		/*
		 * Since there isn't any documentation on what
		 * some of the internal state means, it isn't
		 * clear that this is any different than than
		 * MCP2221_ENGINE_ADDRNACK in the other state
		 * register.
		 */

		if (status_res.internal_i2c_state20 &
		    MCP2221_ENGINE_T1_MASK_NACK) {
			DPRINTF(("umcpmio_i2c_write post check:"
				" engine internal state T1 says NACK\n"));
			err = EIO;
			break;
		}
		if (status_res.internal_i2c_state == 0) {
			DPRINTF(("umcpmio_i2c_write post check:"
				" engine internal state is ZERO\n"));
			err = 0;
			break;
		}
		if (status_res.internal_i2c_state ==
		    MCP2221_ENGINE_WRITINGNOSTOP &&
		    cmd == MCP2221_I2C_WRITE_DATA_NS) {
			DPRINTF(("umcpmio_i2c_write post check:"
				" engine internal state is WRITINGNOSTOP\n"));
			err = 0;
			break;
		}
		if (umcpmio_i2c_fatal(status_res.internal_i2c_state)) {
			DPRINTF(("umcpmio_i2c_write post check:"
				" engine internal state is fatal: %02x\n",
				status_res.internal_i2c_state));
			err = EIO;
			break;
		}
		WAITMS(sc->sc_busy_delay);
	}

 out:
	return err;
}

/*
 * This one deviates a bit from Adafruit in that is supports a straight
 * read and a write + read.  That is, write a register to read from and
 * then do the read.
 */

static int
umcpmio_i2c_read(struct umcpmio_softc *sc, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *databuf, size_t datalen, int
    flags)
{
	struct mcp2221_i2c_req i2c_req;
	struct mcp2221_i2c_res i2c_res;
	struct mcp2221_i2c_fetch_req i2c_fetch_req;
	struct mcp2221_i2c_fetch_res i2c_fetch_res;
	struct mcp2221_status_res status_res;
	int err = 0;
	uint8_t cmd;
	int rretry = sc->sc_retry_busy_read;

	if (cmdbuf != NULL) {
		DPRINTF(("umcpmio_i2c_read: has a cmdbuf, doing write first:"
			" addr=%02x\n", addr));
		err = umcpmio_i2c_write(sc, I2C_OP_WRITE, addr, cmdbuf, cmdlen,
		    NULL, 0, flags);
	}
	if (err)
		goto out;

	err = umcpmio_get_status(sc, &status_res, true);
	if (err)
		goto out;

	if (status_res.internal_i2c_state != 0 &&
	    status_res.internal_i2c_state != MCP2221_ENGINE_WRITINGNOSTOP) {
		DPRINTF(("umcpmio_i2c_read:"
			" internal state not zero and not WRITINGNOSTOP,"
			" clearing. internal_i2c_state=%02x\n",
			status_res.internal_i2c_state));
		err = umcpmio_i2c_clear(sc, true);
	}
	if (err)
		goto out;

	memset(&i2c_req, 0, MCP2221_REQ_BUFFER_SIZE);
	if (cmdbuf == NULL &&
	    status_res.internal_i2c_state != MCP2221_ENGINE_WRITINGNOSTOP) {
		cmd = MCP2221_I2C_READ_DATA;
	} else {
		cmd = MCP2221_I2C_READ_DATA_RS;
	}

	/*
	 * The chip apparently can't do a READ without a STOP
	 * operation.  Report that, and try treating it like a READ
	 * with a STOP.  This won't work for a lot of devices.
	 */

	if (!I2C_OP_STOP_P(op) && sc->sc_reportreadnostop) {
		device_printf(sc->sc_dev,
		    "umcpmio_i2c_read: ************ called with READ"
		    " without STOP ***************\n");
	}

	i2c_req.cmd = cmd;
	i2c_req.lsblen = datalen;
	i2c_req.msblen = 0;
	i2c_req.slaveaddr = (addr << 1) | 0x01;

	DPRINTF(("umcpmio_i2c_read: I2C READ normal read:"
		" cmd=%02x, addr=%02x\n", cmd, addr));

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *)&i2c_req, MCP2221_RES_BUFFER_SIZE,
	    "umcpmio_i2c_read normal read req buffer copy");

	mutex_enter(&sc->sc_action_mutex);
	err = umcpmio_send_report(sc,
	    (uint8_t *)&i2c_req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *)&i2c_res, sc->sc_cv_wait);
	mutex_exit(&sc->sc_action_mutex);

	if (err) {
		device_printf(sc->sc_dev, "umcpmio_i2c_read request error:"
		    " cmd=%02x, err=%d\n", cmd, err);
		err = EIO;
		goto out;
	}

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *)&i2c_res, MCP2221_RES_BUFFER_SIZE,
	    "umcpmio_i2c_read read-request response buffer copy");

	while (rretry > 0) {
		rretry--;
		DPRINTF(("umcpmio_i2c_read: fetch loop: rretry=%d\n", rretry));
		err = 0;
		memset(&i2c_fetch_req, 0, MCP2221_REQ_BUFFER_SIZE);
		i2c_fetch_req.cmd = MCP2221_CMD_I2C_FETCH_READ_DATA;
		mutex_enter(&sc->sc_action_mutex);
		err = umcpmio_send_report(sc,
		    (uint8_t *)&i2c_fetch_req, MCP2221_REQ_BUFFER_SIZE,
		    (uint8_t *)&i2c_fetch_res, sc->sc_cv_wait);
		mutex_exit(&sc->sc_action_mutex);
		umcpmio_dump_buffer(sc->sc_dumpbuffer,
		    (uint8_t *)&i2c_fetch_req, MCP2221_RES_BUFFER_SIZE,
		    "umcpmio_i2c_read fetch res buffer copy");

		if (i2c_fetch_res.cmd != MCP2221_CMD_I2C_FETCH_READ_DATA) {
			device_printf(sc->sc_dev, "umcpmio_i2c_read:"
			    " fetch2: not the command desired: %02x\n",
			    i2c_fetch_res.cmd);
			err = EIO;
			break;
		}
		if (i2c_fetch_res.completion ==
		    MCP2221_FETCH_READ_PARTIALDATA ||
		    i2c_fetch_res.fetchlen == MCP2221_FETCH_READERROR) {
			DPRINTF(("umcpmio_i2c_read: fetch loop:"
				" partial data or read error:"
				" completion=%02x, fetchlen=%02x\n",
				i2c_fetch_res.completion,
				i2c_fetch_res.fetchlen));
			WAITMS(sc->sc_busy_delay);
			err = EAGAIN;
			continue;
		}
		if (i2c_fetch_res.internal_i2c_state ==
		    MCP2221_ENGINE_ADDRNACK) {
			DPRINTF(("umcpmio_i2c_read:"
				" fetch loop: engine NACK\n"));
			err = EIO;
			break;
		}
		if (i2c_fetch_res.internal_i2c_state == 0 &&
		    i2c_fetch_res.fetchlen == 0) {
			DPRINTF(("umcpmio_i2c_read: fetch loop:"
				" internal state and fetch len are ZERO\n"));
			err = 0;
			break;
		}
		if (i2c_fetch_res.internal_i2c_state ==
		    MCP2221_ENGINE_READPARTIAL ||
		    i2c_fetch_res.internal_i2c_state ==
		    MCP2221_ENGINE_READCOMPLETE) {
			DPRINTF(("umcpmio_i2c_read:"
				" fetch loop: read partial or"
				" read complete: internal_i2c_state=%02x\n",
				i2c_fetch_res.internal_i2c_state));
			err = 0;
			break;
		}
	}
	if (err == EAGAIN)
		err = ETIMEDOUT;
	if (err)
		goto out;

	if (databuf == NULL ||
	    i2c_fetch_res.fetchlen == MCP2221_FETCH_READERROR) {
		DPRINTF(("umcpmio_i2c_read: copy data:"
			" databuf is NULL\n"));
		goto out;
	}

	const int size = uimin(i2c_fetch_res.fetchlen, datalen);
	DPRINTF(("umcpmio_i2c_read: copy data: size=%d, fetchlen=%d\n",
		size, i2c_fetch_res.fetchlen));
	if (size > 0) {
		memcpy(databuf, &i2c_fetch_res.data[0], size);
	}

 out:
	return err;
}

static int
umcpmio_i2c_exec(void *v, i2c_op_t op, i2c_addr_t addr, const void *cmdbuf,
    size_t cmdlen, void *databuf, size_t datalen, int flags)
{
	struct umcpmio_softc *sc = v;
	size_t totallen = 0;
	int err = 0;

	if (addr > 0x7f)
		return ENOTSUP;

	if (cmdbuf != NULL)
		totallen += cmdlen;
	if (databuf != NULL)
		totallen += datalen;

	/*
	 * There is a way to do a transfer that is larger than 60 bytes,
	 * but it requires that your break the transfer up into pieces and
	 * send them in 60 byte chunks.  We just won't support that right now.
	 * It would be somewhat unusual for there to be a transfer that big,
	 * unless you are trying to do block transfers and that isn't natively
	 * supported by the chip anyway...  so those have to be broken up and
	 * sent as bytes.
	 */

	if (totallen > 60)
		return ENOTSUP;

	if (I2C_OP_WRITE_P(op)) {
		err = umcpmio_i2c_write(sc, op, addr, cmdbuf, cmdlen,
		    databuf, datalen, flags);

		DPRINTF(("umcpmio_exec: I2C WRITE: err=%d\n", err));
	} else {
		err = umcpmio_i2c_read(sc, op, addr, cmdbuf, cmdlen,
		    databuf, datalen, flags);

		DPRINTF(("umcpmio_exec: I2C READ: err=%d\n", err));
	}

	return err;
}

/* Accessing the ADC and DAC part of the chip */

#define UMCPMIO_DEV_UNIT(m) ((m) & 0x80 ? ((m) & 0x7f) / 3 : (m))
#define UMCPMIO_DEV_WHAT(m) ((m) & 0x80 ? (((m) & 0x7f) % 3) + 1 : CONTROL_DEV)

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

	/*
	 * The control device only allows for ioctl calls, so pretty
	 * much allow any sort of access.  For the ADC, you perform a
	 * strict O_RDONLY and for the DAC a strict O_WRONLY.  It is an
	 * error to try and do a O_RDWR It makes little sense to try
	 * and support select or poll.  The ADC and DAC are always
	 * available for use.
	 */

	if (dunit != CONTROL_DEV &&
	    ((flags & FREAD) && (flags & FWRITE))) {
		DPRINTF(("umcpmio_dev_open: Not CONTROL device and trying to"
			" do READ and WRITE\n"));
		return EINVAL;
	}

	/*
	 * Ya, this unrolling will also have to be changed if the MCP-2210 is
	 * supported.  There are currently only 4 pins, so don't worry too much
	 * about it.  The MCP-2210 has RAM, so there would be a fifth for it.
	 */

	mutex_enter(&sc->sc_action_mutex);
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
			goto out;
		}
		/*
		 * XXX - we can probably do better here...  it
		 * doesn't remember what the pin was set to and
		 * probably should.
		 */
		if (flags & FREAD) {
			error = umcpmio_gpio_pin_ctlctl(sc, pin,
			    GPIO_PIN_ALT0, false);
		} else {
			if (pin == 1) {
				error = EINVAL;
			} else {
				error = umcpmio_gpio_pin_ctlctl(sc,
				    pin, GPIO_PIN_ALT1, false);
			}
		}
	}
	if (error)
		goto out;
	sc->sc_dev_open[dunit] = true;
out:
	mutex_exit(&sc->sc_action_mutex);

	DPRINTF(("umcpmio_dev_open: Opened dunit=%d, pin=%d, error=%d\n",
		dunit, pin, error));

	return error;
}

/* Read an ADC value */

static int
umcpmio_dev_read(dev_t dev, struct uio *uio, int flags)
{
	struct umcpmio_softc *sc;
	struct mcp2221_status_res status_res;
	int dunit;
	int error = 0;
	uint8_t adc_lsb;
	uint8_t adc_msb;
	uint16_t buf;

	sc = device_lookup_private(&umcpmio_cd, UMCPMIO_DEV_UNIT(minor(dev)));
	if (sc == NULL)
		return ENXIO;

	dunit = UMCPMIO_DEV_WHAT(minor(dev));

	if (dunit == CONTROL_DEV) {
		error = EINVAL;
		goto out;
	}

	while (uio->uio_resid && !sc->sc_dying) {
		error = umcpmio_get_status(sc, &status_res, true);
		if (error)
			continue; /* XXX goto out? */
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
			continue; /* XXX goto out? */
		if (sc->sc_dying)
			break;

		buf = adc_msb << 8;
		buf |= adc_lsb;
		error = uiomove(&buf, 2, uio);
		/* XXX missing error test */
	}
out:
	return error;
}

/* Write to the DAC */

static int
umcpmio_dev_write(dev_t dev, struct uio *uio, int flags)
{
	struct umcpmio_softc *sc;
	int dunit;
	int error = 0;

	sc = device_lookup_private(&umcpmio_cd, UMCPMIO_DEV_UNIT(minor(dev)));
	if (sc == NULL)
		return ENXIO;

	dunit = UMCPMIO_DEV_WHAT(minor(dev));

	if (dunit == CONTROL_DEV) {
		error = EINVAL;
		goto out;
	}

	while (uio->uio_resid && !sc->sc_dying) {
		uint8_t buf;

		if ((error = uiomove(&buf, 1, uio)) != 0)
			break;

		if (sc->sc_dying)
			break;

		error = umcpmio_set_dac_value_one(sc, buf, true);
		if (error)
			break;
	}
out:
	return error;
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
			goto out;
			break;
		}
		/*
		 * XXX - Ya, this really could be done better.
		 * Probably should read the sram config and
		 * maybe the gpio config and save out what the
		 * pin was set to.
		 */
		error = umcpmio_gpio_pin_ctlctl(sc, pin,
		    GPIO_PIN_INPUT, false);
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
	struct mcp2221_status_res get_status_res;
	struct mcp2221_get_sram_res get_sram_res;
	struct mcp2221_get_gpio_cfg_res get_gpio_cfg_res;
	struct mcp2221_get_flash_res get_flash_res;
	struct mcp2221_status_res *ioctl_get_status;
	struct mcp2221_get_sram_res *ioctl_get_sram;
	struct mcp2221_get_gpio_cfg_res *ioctl_get_gpio_cfg;
	struct umcpmio_ioctl_get_flash *ioctl_get_flash;
	struct umcpmio_ioctl_put_flash *ioctl_put_flash;
	struct mcp2221_put_flash_req put_flash_req;
	struct mcp2221_put_flash_res put_flash_res;
	int dunit;
	int error = 0;

	sc = device_lookup_private(&umcpmio_cd, UMCPMIO_DEV_UNIT(minor(dev)));
	if (sc->sc_dying)
		return EIO;

	dunit = UMCPMIO_DEV_WHAT(minor(dev));

	if (dunit != CONTROL_DEV) {
		/*
		 * It actually is fine to call ioctl with a unsupported
		 * cmd, but be a little noisy if debug is enabled.
		 */
		DPRINTF(("umcpmio_dev_ioctl: dunit is not the CONTROL device:"
			" dunit=%d, cmd=%ld\n", dunit, cmd));
		return EINVAL;
	}

	mutex_enter(&sc->sc_action_mutex);

	switch (cmd) {
		/*
		 * The GET calls use a shadow buffer for each type of
		 * call.  That probably isn't actually needed and the
		 * memcpy could be avoided.  but...  it is only ever 64
		 * bytes, so maybe not a big deal.
		 */
	case UMCPMIO_GET_STATUS:
		ioctl_get_status = (struct mcp2221_status_res *)data;
		error = umcpmio_get_status(sc, &get_status_res, false);
		umcpmio_dump_buffer(sc->sc_dumpbuffer,
		    (uint8_t *)&get_status_res, MCP2221_RES_BUFFER_SIZE,
		    "umcpmio_dev_ioctl: UMCPMIO_GET_STATUS: get_status_res");
		DPRINTF(("umcpmio_dev_ioctl: UMCPMIO_GET_STATUS:"
			" umcpmio_get_status error=%d\n", error));
		if (error)
			break;
		memcpy(ioctl_get_status, &get_status_res,
		    MCP2221_RES_BUFFER_SIZE);
		break;

	case UMCPMIO_GET_SRAM:
		ioctl_get_sram = (struct mcp2221_get_sram_res *)data;
		error = umcpmio_get_sram(sc, &get_sram_res, false);
		umcpmio_dump_buffer(sc->sc_dumpbuffer,
		    (uint8_t *)&get_sram_res, MCP2221_RES_BUFFER_SIZE,
		    "umcpmio_dev_ioctl: UMCPMIO_GET_SRAM: get_sram_res");
		DPRINTF(("umcpmio_dev_ioctl: UMCPMIO_GET_SRAM:"
			" umcpmio_get_sram error=%d\n", error));
		if (error)
			break;
		memcpy(ioctl_get_sram, &get_sram_res,
		    MCP2221_RES_BUFFER_SIZE);
		break;

	case UMCPMIO_GET_GP_CFG:
		ioctl_get_gpio_cfg = (struct mcp2221_get_gpio_cfg_res *)data;
		error = umcpmio_get_gpio_cfg(sc, &get_gpio_cfg_res, false);
		umcpmio_dump_buffer(sc->sc_dumpbuffer,
		    (uint8_t *)&get_gpio_cfg_res, MCP2221_RES_BUFFER_SIZE,
		    "umcpmio_dev_ioctl: UMCPMIO_GET_GP_CFG: get_gpio_cfg_res");
		DPRINTF(("umcpmio_dev_ioctl: UMCPMIO_GET_GP_CFG:"
			" umcpmio_get_gpio_cfg error=%d\n", error));
		if (error)
			break;
		memcpy(ioctl_get_gpio_cfg, &get_gpio_cfg_res,
		    MCP2221_RES_BUFFER_SIZE);
		break;

	case UMCPMIO_GET_FLASH:
		ioctl_get_flash  = (struct umcpmio_ioctl_get_flash *)data;
		error = umcpmio_get_flash(sc, ioctl_get_flash->subcode,
		    &get_flash_res, false);
		umcpmio_dump_buffer(sc->sc_dumpbuffer,
		    (uint8_t *)&get_flash_res, MCP2221_RES_BUFFER_SIZE,
		    "umcpmio_dev_ioctl: UMCPMIO_GET_FLASH: get_flash_res");
		DPRINTF(("umcpmio_dev_ioctl: UMCPMIO_GET_FLASH:"
			" umcpmio_get_flash subcode=%d, error=%d\n",
			ioctl_get_flash->subcode, error));
		if (error)
			break;
		memcpy(&ioctl_get_flash->get_flash_res, &get_flash_res,
		    MCP2221_RES_BUFFER_SIZE);
		break;

	case UMCPMIO_PUT_FLASH:
		/*
		 * We only allow the flash parts related to gpio to be changed.
		 * Bounce any attempt to do something else.  Also use a shadow
		 * buffer for the put, so we get to control just literally
		 * everything about the write to flash.
		 */
		ioctl_put_flash  = (struct umcpmio_ioctl_put_flash *)data;
		DPRINTF(("umcpmio_dev_ioctl: UMCPMIO_PUT_FLASH:"
			" umcpmio_put_flash subcode=%d\n",
			ioctl_put_flash->subcode));
		if (ioctl_put_flash->subcode != MCP2221_FLASH_SUBCODE_GP) {
			error = EINVAL;
			break;
		}
		memset(&put_flash_req, 0, MCP2221_REQ_BUFFER_SIZE);
		put_flash_req.subcode = ioctl_put_flash->subcode;
		put_flash_req.u.gp.gp0_settings =
		    ioctl_put_flash->put_flash_req.u.gp.gp0_settings;
		put_flash_req.u.gp.gp1_settings =
		    ioctl_put_flash->put_flash_req.u.gp.gp1_settings;
		put_flash_req.u.gp.gp2_settings =
		    ioctl_put_flash->put_flash_req.u.gp.gp2_settings;
		put_flash_req.u.gp.gp3_settings =
		    ioctl_put_flash->put_flash_req.u.gp.gp3_settings;
		umcpmio_dump_buffer(sc->sc_dumpbuffer,
		    (uint8_t *)&ioctl_put_flash->put_flash_req,
		    MCP2221_REQ_BUFFER_SIZE,
		    "umcpmio_dev_ioctl: UMCPMIO_PUT_FLASH:"
		    " ioctl put_flash_req");
		umcpmio_dump_buffer(sc->sc_dumpbuffer,
		    (uint8_t *)&put_flash_req, MCP2221_REQ_BUFFER_SIZE,
		    "umcpmio_dev_ioctl:"
		    " UMCPMIO_PUT_FLASH: put_flash_req");
		memset(&put_flash_res, 0, MCP2221_RES_BUFFER_SIZE);
		error = umcpmio_put_flash(sc, &put_flash_req,
		    &put_flash_res, false);
		/* XXX missing error test? */
		umcpmio_dump_buffer(sc->sc_dumpbuffer,
		    (uint8_t *)&put_flash_res, MCP2221_RES_BUFFER_SIZE,
		    "umcpmio_dev_ioctl: UMCPMIO_PUT_FLASH:"
		    " put_flash_res");
		memcpy(&ioctl_put_flash->put_flash_res, &put_flash_res,
		    MCP2221_RES_BUFFER_SIZE);
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

static struct umcpmio_sysctl_name umcpmio_vref_names[] = {
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
umcpmio_verify_dac_sysctl(SYSCTLFN_ARGS)
{
	char buf[UMCPMIO_VREF_NAME];
	char cbuf[UMCPMIO_VREF_NAME];
	struct umcpmio_softc *sc;
	struct sysctlnode node;
	int error = 0;
	int vrm;
	size_t i;
	struct mcp2221_get_sram_res sram_res;

	node = *rnode;
	sc = node.sysctl_data;

	mutex_enter(&sc->sc_action_mutex);

	error = umcpmio_get_sram(sc, &sram_res, false);
	if (error)
		goto out;

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *)&sram_res, MCP2221_RES_BUFFER_SIZE,
	    "umcpmio_verify_dac_sysctl SRAM res buffer");

	if (sram_res.dac_reference_voltage & MCP2221_SRAM_DAC_IS_VRM) {
		vrm = sram_res.dac_reference_voltage &
		    MCP2221_SRAM_DAC_VRM_MASK;
		switch (vrm) {
		case MCP2221_SRAM_DAC_VRM_4096V:
			strncpy(buf, "4.096V", UMCPMIO_VREF_NAME);
			break;
		case MCP2221_SRAM_DAC_VRM_2048V:
			strncpy(buf, "2.048V", UMCPMIO_VREF_NAME);
			break;
		case MCP2221_SRAM_DAC_VRM_1024V:
			strncpy(buf, "1.024V", UMCPMIO_VREF_NAME);
			break;
		case MCP2221_SRAM_DAC_VRM_OFF:
		default:
			strncpy(buf, "OFF", UMCPMIO_VREF_NAME);
			break;
		}
	} else {
		strncpy(buf, "VDD", UMCPMIO_VREF_NAME);
	}
	strncpy(cbuf, buf, UMCPMIO_VREF_NAME);
	node.sysctl_data = buf;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		goto out;

	for (i = 0; i < __arraycount(umcpmio_vref_names); i++) {
		if (strncmp(node.sysctl_data, umcpmio_vref_names[i].text,
		    UMCPMIO_VREF_NAME) == 0) {
			break;
		}
	}
	if (i == __arraycount(umcpmio_vref_names)) {
		error = EINVAL;
		goto out;
	}

	if (strncmp(cbuf, buf, UMCPMIO_VREF_NAME) == 0) {
		error = 0;
		goto out;
	}

	DPRINTF(("umcpmio_verify_dac_sysctl: setting DAC vref: %s\n", buf));
	error = umcpmio_set_dac_vref_one(sc, buf, false);

 out:
	mutex_exit(&sc->sc_action_mutex);
	return error;
}

int
umcpmio_verify_adc_sysctl(SYSCTLFN_ARGS)
{
	char buf[UMCPMIO_VREF_NAME];
	char cbuf[UMCPMIO_VREF_NAME];
	struct umcpmio_softc *sc;
	struct sysctlnode node;
	int error = 0;
	int vrm;
	size_t i;
	struct mcp2221_get_sram_res sram_res;

	node = *rnode;
	sc = node.sysctl_data;

	mutex_enter(&sc->sc_action_mutex);

	error = umcpmio_get_sram(sc, &sram_res, false);
	if (error)
		goto out;

	if (sram_res.irq_adc_reference_voltage & MCP2221_SRAM_ADC_IS_VRM) {
		vrm = sram_res.irq_adc_reference_voltage &
		    MCP2221_SRAM_ADC_VRM_MASK;
		switch (vrm) {
		case MCP2221_SRAM_ADC_VRM_4096V:
			strncpy(buf, "4.096V", UMCPMIO_VREF_NAME);
			break;
		case MCP2221_SRAM_ADC_VRM_2048V:
			strncpy(buf, "2.048V", UMCPMIO_VREF_NAME);
			break;
		case MCP2221_SRAM_ADC_VRM_1024V:
			strncpy(buf, "1.024V", UMCPMIO_VREF_NAME);
			break;
		case MCP2221_SRAM_ADC_VRM_OFF:
		default:
			strncpy(buf, "OFF", UMCPMIO_VREF_NAME);
			break;
		}
	} else {
		strncpy(buf, "VDD", UMCPMIO_VREF_NAME);
	}
	strncpy(cbuf, buf, UMCPMIO_VREF_NAME);
	node.sysctl_data = buf;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		goto out;

	for (i = 0; i < __arraycount(umcpmio_vref_names); i++) {
		if (strncmp(node.sysctl_data, umcpmio_vref_names[i].text,
		    UMCPMIO_VREF_NAME) == 0) {
			break;
		}
	}
	if (i == __arraycount(umcpmio_vref_names)) {
		error = EINVAL;
		goto out;
	}

	if (strncmp(cbuf, buf, UMCPMIO_VREF_NAME) == 0) {
		error = 0;
		goto out;
	}

	DPRINTF(("umcpmio_verify_adc_sysctl: setting ADC vref: %s\n", buf));
	error = umcpmio_set_adc_vref_one(sc, buf, false);

 out:
	mutex_exit(&sc->sc_action_mutex);
	return error;
}

static struct umcpmio_sysctl_name umcpmio_dc_names[] = {
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
umcpmio_verify_gpioclock_dc_sysctl(SYSCTLFN_ARGS)
{
	char buf[UMCPMIO_VREF_NAME];
	char cbuf[UMCPMIO_VREF_NAME];
	struct umcpmio_softc *sc;
	struct sysctlnode node;
	int error = 0;
	uint8_t duty_cycle;
	size_t i;
	struct mcp2221_get_sram_res sram_res;

	node = *rnode;
	sc = node.sysctl_data;

	mutex_enter(&sc->sc_action_mutex);

	error = umcpmio_get_sram(sc, &sram_res, false);
	if (error)
		goto out;

	duty_cycle = sram_res.clock_divider & MCP2221_SRAM_GPIO_CLOCK_DC_MASK;
	DPRINTF(("umcpmio_verify_gpioclock_dc_sysctl: current duty cycle:"
		" %02x\n", duty_cycle));
	switch (duty_cycle) {
	case MCP2221_SRAM_GPIO_CLOCK_DC_75:
		strncpy(buf, "75%", UMCPMIO_DC_NAME);
		break;
	case MCP2221_SRAM_GPIO_CLOCK_DC_50:
		strncpy(buf, "50%", UMCPMIO_DC_NAME);
		break;
	case MCP2221_SRAM_GPIO_CLOCK_DC_25:
		strncpy(buf, "25%", UMCPMIO_DC_NAME);
		break;
	case MCP2221_SRAM_GPIO_CLOCK_DC_0:
	default:
		strncpy(buf, "0%", UMCPMIO_DC_NAME);
		break;
	}
	strncpy(cbuf, buf, UMCPMIO_VREF_NAME);
	node.sysctl_data = buf;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		goto out;

	for (i = 0; i < __arraycount(umcpmio_dc_names); i++) {
		if (strncmp(node.sysctl_data, umcpmio_dc_names[i].text,
		    UMCPMIO_VREF_NAME) == 0) {
			break;
		}
	}
	if (i == __arraycount(umcpmio_dc_names)) {
		error = EINVAL;
		goto out;
	}

	if (strncmp(cbuf, buf, UMCPMIO_VREF_NAME) == 0) {
		error = 0;
		goto out;
	}

	DPRINTF(("umcpmio_verify_gpioclock_dc_sysctl:"
		" setting GPIO clock duty cycle: %s\n", buf));
	error = umcpmio_set_gpioclock_dc_one(sc, buf, false);

 out:
	mutex_exit(&sc->sc_action_mutex);
	return error;
}

static struct umcpmio_sysctl_name umcpmio_cd_names[] = {
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
umcpmio_verify_gpioclock_cd_sysctl(SYSCTLFN_ARGS)
{
	char buf[UMCPMIO_CD_NAME];
	char cbuf[UMCPMIO_CD_NAME];
	struct umcpmio_softc *sc;
	struct sysctlnode node;
	int error = 0;
	uint8_t clock_divider;
	size_t i;
	struct mcp2221_get_sram_res sram_res;

	node = *rnode;
	sc = node.sysctl_data;

	mutex_enter(&sc->sc_action_mutex);

	error = umcpmio_get_sram(sc, &sram_res, false);
	if (error)
		goto out;

	clock_divider = sram_res.clock_divider &
	    MCP2221_SRAM_GPIO_CLOCK_CD_MASK;
	DPRINTF(("umcpmio_verify_gpioclock_cd_sysctl: current clock divider:"
		" %02x\n", clock_divider));
	switch (clock_divider) {
	case MCP2221_SRAM_GPIO_CLOCK_CD_375KHZ:
		strncpy(buf, "375kHz", UMCPMIO_CD_NAME);
		break;
	case MCP2221_SRAM_GPIO_CLOCK_CD_750KHZ:
		strncpy(buf, "750kHz", UMCPMIO_CD_NAME);
		break;
	case MCP2221_SRAM_GPIO_CLOCK_CD_1P5MHZ:
		strncpy(buf, "1.5MHz", UMCPMIO_CD_NAME);
		break;
	case MCP2221_SRAM_GPIO_CLOCK_CD_3MHZ:
		strncpy(buf, "3MHz", UMCPMIO_CD_NAME);
		break;
	case MCP2221_SRAM_GPIO_CLOCK_CD_6MHZ:
		strncpy(buf, "6MHz", UMCPMIO_CD_NAME);
		break;
	case MCP2221_SRAM_GPIO_CLOCK_CD_12MHZ:
		strncpy(buf, "12MHz", UMCPMIO_CD_NAME);
		break;
	case MCP2221_SRAM_GPIO_CLOCK_CD_24MHZ:
		strncpy(buf, "24MHz", UMCPMIO_CD_NAME);
		break;
	default:
		strncpy(buf, "12MHz", UMCPMIO_CD_NAME);
		break;
	}
	strncpy(cbuf, buf, UMCPMIO_CD_NAME);
	node.sysctl_data = buf;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		goto out;

	for (i = 0; i < __arraycount(umcpmio_cd_names); i++) {
		if (strncmp(node.sysctl_data, umcpmio_cd_names[i].text,
		    UMCPMIO_CD_NAME) == 0) {
			break;
		}
	}
	if (i == __arraycount(umcpmio_cd_names)) {
		error = EINVAL;
		goto out;
	}

	if (strncmp(cbuf, buf, UMCPMIO_CD_NAME) == 0) {
		error = 0;
		goto out;
	}

	DPRINTF(("umcpmio_verify_gpioclock_cd_sysctl:"
		" setting GPIO clock clock divider: %s\n",
		buf));
	error = umcpmio_set_gpioclock_cd_one(sc, buf, false);

 out:
	mutex_exit(&sc->sc_action_mutex);
	return error;
}

static int
umcpmio_sysctl_init(struct umcpmio_softc *sc)
{
	int error;
	const struct sysctlnode *cnode;
	int sysctlroot_num, i2c_num, adc_dac_num, adc_num, dac_num, gpio_num;

	if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
	    0, CTLTYPE_NODE, device_xname(sc->sc_dev),
	    SYSCTL_DESCR("mcpmio controls"),
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

	if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
	    0, CTLTYPE_NODE, "gpio",
	    SYSCTL_DESCR("GPIO controls"),
	    NULL, 0, NULL, 0,
	    CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	gpio_num = cnode->sysctl_num;

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

	/* GPIO */
	if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "irq_poll",
	    SYSCTL_DESCR("How often to poll for a IRQ change"),
	    umcpmio_verify_sysctl, 0, &sc->sc_irq_poll, 0,
	    CTL_HW, sysctlroot_num, gpio_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

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
	    umcpmio_verify_gpioclock_dc_sysctl, 0, (void *)sc, UMCPMIO_DC_NAME,
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
	    umcpmio_verify_gpioclock_cd_sysctl, 0, (void *)sc, UMCPMIO_CD_NAME,
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
	    umcpmio_verify_adc_sysctl, 0, (void *)sc, UMCPMIO_VREF_NAME,
	    CTL_HW, sysctlroot_num, adc_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	/* DAC */
	if ((error = sysctl_createv(&sc->sc_umcpmiolog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_STRING, "vref",
	    SYSCTL_DESCR("DAC voltage reference"),
	    umcpmio_verify_dac_sysctl, 0, (void *)sc, UMCPMIO_VREF_NAME,
	    CTL_HW, sysctlroot_num, dac_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	return 0;
}

static int
umcpmio_match(device_t parent, cfdata_t match, void *aux)
{
	struct uhidev_attach_arg *uha = aux;

	return umcpmio_lookup(uha->uiaa->uiaa_vendor, uha->uiaa->uiaa_product)
	    != NULL ? UMATCH_VENDOR_PRODUCT : UMATCH_NONE;
}

/*
 * This driver could be extended to support the MCP-2210 which is MCP's
 * USB to SPI / gpio chip.  It also appears to be a something like the
 * PIC16F1455 used in the MCP2221 / MCP2221A.  It is likely that a lot
 * of this could use tables to drive behavior.
 */

static void
umcpmio_attach(device_t parent, device_t self, void *aux)
{
	struct umcpmio_softc *sc = device_private(self);
	struct uhidev_attach_arg *uha = aux;
	struct gpiobus_attach_args gba;
	struct i2cbus_attach_args iba;
	struct mcp2221_status_res status_res;
	int err;

	sc->sc_dev = self;
	sc->sc_hdev = uha->parent;
	sc->sc_udev = uha->uiaa->uiaa_device;

	sc->sc_umcpmiolog = NULL;
	sc->sc_dumpbuffer = false;

	sc->sc_reportreadnostop = true;
	sc->sc_cv_wait = 2500;
	sc->sc_response_errcnt = 5;
	sc->sc_busy_delay = 1;
	sc->sc_retry_busy_read = 50;
	sc->sc_retry_busy_write = 50;
	sc->sc_irq_poll = 10;
	sc->sc_dev_open[CONTROL_DEV] = false;
	sc->sc_dev_open[GP1_DEV] = false;
	sc->sc_dev_open[GP2_DEV] = false;
	sc->sc_dev_open[GP3_DEV] = false;

	aprint_normal("\n");

	if ((err = umcpmio_sysctl_init(sc)) != 0) {
		aprint_error_dev(self, "Can't setup sysctl tree (%d)\n", err);
		return;
	}

	mutex_init(&sc->sc_action_mutex, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->sc_res_cv, "mcpres");
	mutex_init(&sc->sc_res_mutex, MUTEX_DEFAULT, IPL_NONE);
	sc->sc_res_buffer = NULL;
	sc->sc_res_ready = false;

	err = uhidev_open(sc->sc_hdev, &umcpmio_uhidev_intr, sc);
	if (err) {
		aprint_error_dev(sc->sc_dev, "umcpmio_attach: "
		    " uhidev_open: err=%d\n", err);
		return;
	}

	/*
	 * It is not clear that this should be needed, but it was noted
	 * that the device would sometimes not be ready if this delay
	 * was not present.  In fact, the attempts to set stuff a
	 * little later would sometimes fail.
	 */

	delay(1000);

	err = umcpmio_get_status(sc, &status_res, true);
	if (err) {
		aprint_error_dev(sc->sc_dev, "umcpmio_attach: "
		    " umcpmio_get_status: err=%d\n", err);
		return;
	}

	aprint_normal_dev(sc->sc_dev,
	    "Hardware revision: %d.%d, Firmware revision: %d.%d\n",
	    status_res.mcp2221_hardware_rev_major,
	    status_res.mcp2221_hardware_rev_minor,
	    status_res.mcp2221_firmware_rev_major,
	    status_res.mcp2221_firmware_rev_minor);

	/*
	 * The datasheet suggests that it is possble for this
	 * to fail if the I2C port is currently being used.
	 * However...  since you just plugged in the chip, the
	 * I2C port should not really be in use at that moment.
	 * In any case, try hard to set this and don't make it
	 * fatal if it did not get set.
	 */
	int i2cspeed;
	for (i2cspeed = 0; i2cspeed < 3; i2cspeed++) {
		err = umcpmio_set_i2c_speed_one(sc, I2C_SPEED_SM, true);
		if (err) {
			aprint_error_dev(sc->sc_dev, "umcpmio_attach:"
			    " set I2C speed: err=%d\n",
			    err);
			delay(300);
		}
		break;
	}

	struct mcp2221_get_sram_res get_sram_res;
	err = umcpmio_get_sram(sc, &get_sram_res, true);
	if (err) {
		aprint_error_dev(sc->sc_dev, "umcpmio_attach:"
		    " get sram error: err=%d\n",
		    err);
		return;
	}

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *)&get_sram_res, MCP2221_RES_BUFFER_SIZE,
	    "umcpmio_attach get sram buffer copy");

	/*
	 * There are only 4 pins right now, just unroll
	 * any loops
	 */

	sc->sc_gpio_pins[0].pin_num = 0;
	sc->sc_gpio_pins[0].pin_caps = GPIO_PIN_INPUT;
	sc->sc_gpio_pins[0].pin_caps |= GPIO_PIN_OUTPUT;
	sc->sc_gpio_pins[0].pin_caps |= GPIO_PIN_ALT0;
	sc->sc_gpio_pins[0].pin_caps |= GPIO_PIN_ALT3;
	sc->sc_gpio_pins[0].pin_flags =
	    umcpmio_sram_gpio_to_flags(get_sram_res.gp0_settings);
	sc->sc_gpio_pins[0].pin_intrcaps = 0;
	snprintf(sc->sc_gpio_pins[0].pin_defname, 4, "GP0");

	sc->sc_gpio_pins[1].pin_num = 1;
	sc->sc_gpio_pins[1].pin_caps = GPIO_PIN_INPUT;
	sc->sc_gpio_pins[1].pin_caps |= GPIO_PIN_OUTPUT;
	sc->sc_gpio_pins[1].pin_caps |= GPIO_PIN_ALT0;
	sc->sc_gpio_pins[1].pin_caps |= GPIO_PIN_ALT1;
	sc->sc_gpio_pins[1].pin_caps |= GPIO_PIN_ALT2;
	sc->sc_gpio_pins[1].pin_caps |= GPIO_PIN_ALT3;
	sc->sc_gpio_pins[1].pin_flags =
	    umcpmio_sram_gpio_to_flags(get_sram_res.gp1_settings);
	/* XXX - lets not advertise this right now... */
#if 0
	sc->sc_gpio_pins[1].pin_intrcaps = GPIO_INTR_POS_EDGE;
	sc->sc_gpio_pins[1].pin_intrcaps |= GPIO_INTR_NEG_EDGE;
	sc->sc_gpio_pins[1].pin_intrcaps |= GPIO_INTR_DOUBLE_EDGE;
	sc->sc_gpio_pins[1].pin_intrcaps |= GPIO_INTR_MPSAFE;
#endif
	sc->sc_gpio_pins[1].pin_intrcaps = 0;
	snprintf(sc->sc_gpio_pins[1].pin_defname, 4, "GP1");

	sc->sc_gpio_pins[2].pin_num = 2;
	sc->sc_gpio_pins[2].pin_caps = GPIO_PIN_INPUT;
	sc->sc_gpio_pins[2].pin_caps |= GPIO_PIN_OUTPUT;
	sc->sc_gpio_pins[2].pin_caps |= GPIO_PIN_ALT0;
	sc->sc_gpio_pins[2].pin_caps |= GPIO_PIN_ALT1;
	sc->sc_gpio_pins[2].pin_caps |= GPIO_PIN_ALT3;
	sc->sc_gpio_pins[2].pin_flags =
	    umcpmio_sram_gpio_to_flags(get_sram_res.gp2_settings);
	sc->sc_gpio_pins[2].pin_intrcaps = 0;
	snprintf(sc->sc_gpio_pins[2].pin_defname, 4, "GP2");

	sc->sc_gpio_pins[3].pin_num = 3;
	sc->sc_gpio_pins[3].pin_caps = GPIO_PIN_INPUT;
	sc->sc_gpio_pins[3].pin_caps |= GPIO_PIN_OUTPUT;
	sc->sc_gpio_pins[3].pin_caps |= GPIO_PIN_ALT0;
	sc->sc_gpio_pins[3].pin_caps |= GPIO_PIN_ALT1;
	sc->sc_gpio_pins[3].pin_caps |= GPIO_PIN_ALT3;
	sc->sc_gpio_pins[3].pin_flags =
	    umcpmio_sram_gpio_to_flags(get_sram_res.gp3_settings);
	sc->sc_gpio_pins[3].pin_intrcaps = 0;
	snprintf(sc->sc_gpio_pins[3].pin_defname, 4, "GP3");

	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = umcpmio_gpio_pin_read;
	sc->sc_gpio_gc.gp_pin_write = umcpmio_gpio_pin_write;
	sc->sc_gpio_gc.gp_pin_ctl = umcpmio_gpio_pin_ctl;

	sc->sc_gpio_gc.gp_intr_establish = umcpmio_gpio_intr_establish;
	sc->sc_gpio_gc.gp_intr_disestablish = umcpmio_gpio_intr_disestablish;
	sc->sc_gpio_gc.gp_intr_str = umcpmio_gpio_intrstr;

	gba.gba_gc = &sc->sc_gpio_gc;
	gba.gba_pins = sc->sc_gpio_pins;
	gba.gba_npins = MCP2221_NPINS;

	sc->sc_gpio_dev = config_found(self, &gba, gpiobus_print,
	    CFARGS(.iattr = "gpiobus"));

	iic_tag_init(&sc->sc_i2c_tag);
	sc->sc_i2c_tag.ic_cookie = sc;
	sc->sc_i2c_tag.ic_acquire_bus = umcpmio_acquire_bus;
	sc->sc_i2c_tag.ic_release_bus = umcpmio_release_bus;
	sc->sc_i2c_tag.ic_exec = umcpmio_i2c_exec;

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_i2c_tag;
	sc->sc_i2c_dev = config_found(self, &iba, iicbus_print,
	    CFARGS(.iattr = "i2cbus"));
}

static int
umcpmio_detach(device_t self, int flags)
{
	struct umcpmio_softc *sc = device_private(self);
	int err;

	DPRINTF(("umcpmio_detach: sc=%p flags=%d\n", sc, flags));

	mutex_enter(&sc->sc_action_mutex);
	sc->sc_dying = 1;

	err = config_detach_children(self, flags);
	if (err)
		return err;

	uhidev_close(sc->sc_hdev);

	mutex_destroy(&sc->sc_res_mutex);
	cv_destroy(&sc->sc_res_cv);

	sysctl_teardown(&sc->sc_umcpmiolog);

	mutex_exit(&sc->sc_action_mutex);
	mutex_destroy(&sc->sc_action_mutex);

	return 0;
}

static int
umcpmio_activate(device_t self, enum devact act)
{
	struct umcpmio_softc *sc = device_private(self);

	DPRINTFN(5, ("umcpmio_activate: %d\n", act));

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

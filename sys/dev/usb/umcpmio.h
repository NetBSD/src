/*	$NetBSD: umcpmio.h,v 1.2 2025/03/17 18:24:08 riastradh Exp $	*/

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

#ifndef _UMCPMIO_H_
#define _UMCPMIO_H_

#include <sys/param.h>
#include <sys/types.h>

#include <sys/condvar.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/gpio.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/mutex.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/vnode.h>

#include <dev/gpio/gpiovar.h>

#include <dev/hid/hid.h>

#include <dev/i2c/i2cvar.h>

#include <dev/usb/uhidev.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>

#define UMCPMIO_VREF_NAME 7
#define UMCPMIO_CD_NAME 7
#define UMCPMIO_DC_NAME 4

#define MCP2221_NPINS 4
#define UMCPMIO_NUM_DEVS 4

enum umcpmio_minor_devs {
	CONTROL_DEV = 0,
	GP1_DEV = 1,
	GP2_DEV = 2,
	GP3_DEV = 3,
};

struct umcpmio_irq {
	int (*sc_gpio_irqfunc)(void *);
	void *sc_gpio_irqarg;
};

struct umcpmio_softc {
	device_t		sc_dev;
	struct uhidev		*sc_hdev;
	struct usbd_device	*sc_udev;

	struct sysctllog	*sc_umcpmiolog;
	bool			sc_dumpbuffer;

	int			sc_cv_wait;
	int			sc_response_errcnt;
	int			sc_busy_delay;
	int			sc_retry_busy_read;
	int			sc_retry_busy_write;

	kmutex_t		sc_action_mutex;

	kcondvar_t		sc_res_cv;
	kmutex_t		sc_res_mutex;
	bool			sc_res_ready;
	uint8_t			*sc_res_buffer;

	device_t		sc_gpio_dev;
	struct gpio_chipset_tag	sc_gpio_gc;
	gpio_pin_t		sc_gpio_pins[MCP2221_NPINS];
        struct umcpmio_irq      sc_gpio_irqs[1];
	int			sc_irq_poll;

	struct i2c_controller	sc_i2c_tag;
	device_t		sc_i2c_dev;
	bool			sc_reportreadnostop;

	bool			sc_dev_open[UMCPMIO_NUM_DEVS];

	char			sc_dying;
};

struct umcpmio_sysctl_name {
	const char	*text;
};

#endif	/* _UMCPMIO_H_ */

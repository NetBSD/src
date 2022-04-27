/*	$NetBSD: apple_mbox.c,v 1.1 2022/04/27 08:06:20 skrll Exp $	*/
/*	$OpenBSD: apple_mbox.c,v 1.2 2022/01/04 20:55:48 kettenis Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
 * Copyright (c) 2021 Mark Kettenis <kettenis@openbsd.org>
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
__KERNEL_RCSID(0, "$NetBSD: apple_mbox.c,v 1.1 2022/04/27 08:06:20 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

#include <arm/apple/apple_mbox.h>

#define MBOX_A2I_CTRL		0x110
#define  MBOX_A2I_CTRL_FULL	__BIT(16)
#define MBOX_I2A_CTRL		0x114
#define  MBOX_I2A_CTRL_EMPTY	__BIT(17)
#define MBOX_A2I_SEND0		0x800
#define MBOX_A2I_SEND1		0x808
#define MBOX_I2A_RECV0		0x830
#define MBOX_I2A_RECV1		0x838

#define MBOX_READ4(sc, reg)							\
	(bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg)))
#define MBOX_READ8(sc, reg)							\
	(bus_space_read_8((sc)->sc_bst, (sc)->sc_bsh, (reg)))
#define MBOX_WRITE4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define MBOX_WRITE8(sc, reg, val)						\
	bus_space_write_8((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int apple_mbox_intr(void *);

static struct mbox_interrupt {
	const char *mi_name;
	bool mi_claim;
	int (*mi_handler)(void *);
} mbox_interrupts[] = {
	{
		.mi_name = "send-empty",
		.mi_claim = false,
		.mi_handler = apple_mbox_intr,
	},
	{
		.mi_name = "send-not-empty",
		.mi_claim = false,
	},
	{
		.mi_name = "recv-empty",
		.mi_claim = false,
	},
	{
		.mi_name = "recv-not-empty",
		.mi_claim = true,
		.mi_handler = apple_mbox_intr,
	},
};

struct apple_mbox_softc {
	device_t sc_dev;
	int sc_phandle;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;

	void *sc_intr[__arraycount(mbox_interrupts)];
	void (*sc_rx_callback)(void *);
	void *sc_rx_arg;

//	struct fdtbus_mbox_device sc_md;
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "apple,asc-mailbox" },
	{ .compat = "apple,asc-mailbox-v4" },
	DEVICE_COMPAT_EOL
};


static int
apple_mbox_intr(void *arg)
{
	struct apple_mbox_softc const *sc = arg;
	const uint32_t ctrl = MBOX_READ4(sc, MBOX_I2A_CTRL);

	if (ctrl & MBOX_I2A_CTRL_EMPTY)
		return 0;

	if (sc->sc_rx_callback) {
		sc->sc_rx_callback(sc->sc_rx_arg);
	} else {
		printf("%s: 0x%016" PRIx64 "0x%016" PRIx64 "\n",
		    device_xname(sc->sc_dev),
		    MBOX_READ8(sc, MBOX_I2A_RECV0),
		    MBOX_READ8(sc, MBOX_I2A_RECV1));
	}

	return 1;
}

static void *
apple_mbox_acquire(device_t dev, const void *cells, size_t len,
    void (*cb)(void *), void *arg)
{
	struct apple_mbox_softc * const sc = device_private(dev);

	if (sc->sc_rx_callback == NULL && sc->sc_rx_arg == NULL) {
		sc->sc_rx_callback = cb;
		sc->sc_rx_arg = arg;

		return sc;
	}

	return NULL;
}

static void
apple_mbox_release(device_t dev, void *priv)
{
	struct apple_mbox_softc * const sc = device_private(dev);

	KASSERT(sc == priv);

	sc->sc_rx_callback = NULL;
	sc->sc_rx_arg = NULL;
}

static int
apple_mbox_send(device_t dev, void *priv, const void *data, size_t len)
{
	struct apple_mbox_softc * const sc = device_private(dev);
	const struct apple_mbox_msg *msg = data;

	KASSERT(sc == priv);

	if (len != sizeof(struct apple_mbox_msg))
		return EINVAL;


	uint32_t ctrl = MBOX_READ4(sc, MBOX_A2I_CTRL);
	if (ctrl & MBOX_A2I_CTRL_FULL)
		return EBUSY;

	MBOX_WRITE8(sc, MBOX_A2I_SEND0, msg->data0);
	MBOX_WRITE8(sc, MBOX_A2I_SEND1, msg->data1);

	return 0;
}

static int
apple_mbox_recv(device_t dev, void *priv, void *data, size_t len)
{
	struct apple_mbox_softc * const sc = device_private(dev);
	struct apple_mbox_msg *msg = data;

	KASSERT(sc == priv);
	if (len != sizeof(struct apple_mbox_msg))
		return EINVAL;

	uint32_t ctrl = MBOX_READ4(sc, MBOX_I2A_CTRL);
	if (ctrl & MBOX_I2A_CTRL_EMPTY)
		return EAGAIN;

	msg->data0 = MBOX_READ8(sc, MBOX_I2A_RECV0);
	msg->data1 = MBOX_READ8(sc, MBOX_I2A_RECV1);

	return 0;
}


static int
apple_mbox_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
apple_mbox_attach(device_t parent, device_t self, void *aux)
{
	struct apple_mbox_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_rx_callback = NULL;
	sc->sc_rx_arg = NULL;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Apple Mailbox\n");

	for (size_t i = 0; i < __arraycount(mbox_interrupts); i++) {
		struct mbox_interrupt *mi = &mbox_interrupts[i];

		if (!mi->mi_claim)
			continue;

		int index;
		int err = fdtbus_get_index(phandle, "interrupt-names",
		    mi->mi_name, &index);
		if (err != 0) {
			aprint_error_dev(self,
			    "couldn't get %s interrupt index\n", mi->mi_name);
			continue;
		}

		char istr[128];
		if (!fdtbus_intr_str(phandle, index, istr, sizeof(istr))) {
			aprint_error_dev(self,
			    "couldn't decode %s interrupt\n", mi->mi_name);
			continue;
		}

		sc->sc_intr[i] = fdtbus_intr_establish_xname(phandle, index,
		    IPL_VM, FDT_INTR_MPSAFE, mi->mi_handler, sc,
		    device_xname(self));
		if (sc->sc_intr[i] == NULL) {
			aprint_error_dev(self,
			    "couldn't establish %s interrupt\n", mi->mi_name);
			continue;
		}

		aprint_normal_dev(self, "'%s' interrupting on %s\n",
		    mi->mi_name, istr);
	}

	static struct fdtbus_mbox_controller_func funcs = {
		.mc_acquire = apple_mbox_acquire,
		.mc_release = apple_mbox_release,
		.mc_send = apple_mbox_send,
		.mc_recv = apple_mbox_recv,
	};

	int error = fdtbus_register_mbox_controller(self, phandle, &funcs);
	if (error) {
		aprint_error_dev(self, "couldn't register mailbox\n");
		goto fail_register;
	}
	return;

fail_register:
	for (size_t i = 0; i < __arraycount(mbox_interrupts); i++) {
		if (sc->sc_intr[i] != NULL) {
			fdtbus_intr_disestablish(phandle, sc->sc_intr[i]);
		}
	}

	return;
}

CFATTACH_DECL_NEW(apple_mbox, sizeof(struct apple_mbox_softc),
    apple_mbox_match, apple_mbox_attach, NULL, NULL);

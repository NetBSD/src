/* $NetBSD: fdt_spi.c,v 1.3.2.1 2021/08/09 00:30:09 thorpej Exp $ */

/*
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tobias Nygren.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fdt_spi.c,v 1.3.2.1 2021/08/09 00:30:09 thorpej Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/queue.h>
#include <dev/spi/spivar.h>
#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

struct fdtbus_spi_controller {
	struct spi_controller *spi_ctlr;
	int spi_phandle;

	LIST_ENTRY(fdtbus_spi_controller) spi_next;
};

static LIST_HEAD(, fdtbus_spi_controller) fdtbus_spi_controllers =
    LIST_HEAD_INITIALIZER(fdtbus_spi_controllers);

int
fdtbus_register_spi_controller(struct spi_controller *ctlr, int phandle)
{
	struct fdtbus_spi_controller *spi;

	spi = kmem_alloc(sizeof(*spi), KM_SLEEP);
	spi->spi_ctlr = ctlr;
	spi->spi_phandle = phandle;

	LIST_INSERT_HEAD(&fdtbus_spi_controllers, spi, spi_next);

	return 0;
}

#if 0
static struct spi_controller *
fdtbus_get_spi_controller(int phandle)
{
	struct fdtbus_spi_controller *spi;

	LIST_FOREACH(spi, &fdtbus_spi_controllers, spi_next) {
		if (spi->spi_phandle == phandle) {
			return spi->spi_ctlr;
		}
	}
	return NULL;
}
#endif

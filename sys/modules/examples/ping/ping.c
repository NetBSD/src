/*	$NetBSD: ping.c,v 1.1.20.2 2017/12/03 11:38:53 jdolecek Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ping.c,v 1.1.20.2 2017/12/03 11:38:53 jdolecek Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/module.h>

#include "ping.h"

/*
 * Create a device /dev/ping in order to test this module.
 *
 * To use this device you need to do:
 *     mknod /dev/ping c 210 0
 *
 */

dev_type_open(ping_open);
dev_type_close(ping_close);
dev_type_ioctl(ping_ioctl);

static struct cdevsw ping_cdevsw = {
	.d_open = ping_open,
	.d_close = ping_close,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = ping_ioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};


struct ping_softc {
	int		refcnt;
};

static struct ping_softc sc;

int
ping_open(dev_t self __unused, int flag __unused, int mode __unused,
          struct lwp *l __unused)
{
	if (sc.refcnt > 0)
		return EBUSY;

	++sc.refcnt;

	return 0;
}

int
ping_close(dev_t self __unused, int flag __unused, int mode __unused,
           struct lwp *l __unused)
{
	--sc.refcnt;

	return 0;
}

int
ping_ioctl(dev_t self __unused, u_long cmd, void *data, int flag,
           struct lwp *l __unused)
{
	switch(cmd) {
	case CMD_PING:
		printf("ping: pong!\n");
		return 0;
	default:
		return 1;
	}
}

MODULE(MODULE_CLASS_MISC, ping, NULL);

static int
ping_modcmd(modcmd_t cmd, void *arg __unused)
{
	/* The major should be verified and changed if needed to avoid
	 * conflicts with other devices. */
	int cmajor = 210, bmajor = -1;

	switch (cmd) {
	case MODULE_CMD_INIT:
		if (devsw_attach("ping", NULL, &bmajor, &ping_cdevsw, &cmajor))
			return ENXIO;
		return 0;
	case MODULE_CMD_FINI:
		if (sc.refcnt > 0)
			return EBUSY;

		devsw_detach(NULL, &ping_cdevsw);
		return 0;
	default:
		return ENOTTY;
	}
}

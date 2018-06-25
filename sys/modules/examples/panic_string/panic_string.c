/*	$NetBSD: panic_string.c,v 1.1.2.2 2018/06/25 07:26:05 pgoyette Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: panic_string.c,v 1.1.2.2 2018/06/25 07:26:05 pgoyette Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/module.h>
#include <sys/vfs_syscalls.h>

/*
 * Create a device /dev/panic from which you can read sequential
 * user input.
 *
 * To use this device you need to do:
 *      mknod /dev/panic c 210 0
 *
 * To write to the device you might need:
 *      chmod 666 /dev/panic
 *
 * Commentary:
 * This module manages the device /dev/panic,
 * tranfers a string from userspace to kernel space
 * and calls kernel panic with the passed string.
 *
 *  echo 'string' > /dev/panic
 * will do the trick after loading the module.
 */

dev_type_open(panic_string_open);
dev_type_close(panic_string_close);
dev_type_write(panic_string_write);

static struct cdevsw panic_string_cdevsw = {
	.d_open = panic_string_open,
	.d_close = panic_string_close,
	.d_read = noread,
	.d_write = panic_string_write,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

static struct panic_string_softc {
	int refcnt;
} sc;

/*
 * A function similar to strnlen + isprint
 *
 * Detect length of the printable and non-whitespace string in the buffer.
 * A string is accepted if it contains any non-space character.
 */

static size_t
printable_length(const char *str, size_t len)
{
	size_t n;	
	bool accepted;

	n = 0;
	accepted = false;

	while (len > n) {
		if (str[n] >= 0x20 && str[n] <= 0x7e) {
			if (str[n] != 0x20 /* space */)
				accepted = true;
			n++;
		} else
			break;
	}

	if (accepted)
		return n;
	else
		return 0;
}

int
panic_string_open(dev_t self __unused, int flag __unused, int mod __unused, struct lwp *l)
{

	/* Make sure the device is opened once at a time */
	if (sc.refcnt > 0)
		return EBUSY;

	++sc.refcnt;

	return 0;
}

int
panic_string_close(dev_t self __unused, int flag __unused, int mod __unused, struct lwp *l __unused)
{

	--sc.refcnt;
	return 0;
}

int
panic_string_write(dev_t self, struct uio *uio, int flags)
{
	size_t len, printlen;
	char *buffer;

	/* Buffer length */
	len = uio->uio_iov->iov_len;

	/* Allocate a local buffer to store the string */
	buffer = (char *)kmem_alloc(len, KM_SLEEP);

	/* Move the string from user to kernel space and store it locally */
	uiomove(buffer, len, uio);

	printlen = printable_length(buffer, len);

	if (printlen > 0) {
		/* Flushing disk changes */
		do_sys_sync(curlwp);

		panic("panic string: %.*s\n", (int)printlen, buffer);
//		printf("panic string: %.*s\n", (int)printlen, buffer);

		/* NOTREACHED */
	}

	kmem_free(buffer, len);
	return 0;
}

MODULE(MODULE_CLASS_MISC, panic_string, NULL);

static int
panic_string_modcmd(modcmd_t cmd, void *arg __unused)
{
	/* The major should be verified and changed if needed to avoid
	 * conflicts with other devices. */
	int cmajor = 210, bmajor = -1;

	switch (cmd) {
	case MODULE_CMD_INIT:
		printf("Panic String module loaded.\n");
		if (devsw_attach("panic", NULL, &bmajor, &panic_string_cdevsw,
						 &cmajor))
			return ENXIO;
		return 0;

	case MODULE_CMD_FINI:
		printf("Panic String module unloaded.\n");
		if (sc.refcnt > 0)
			return EBUSY;

		devsw_detach(NULL, &panic_string_cdevsw);
		return 0;
	default:
		return ENOTTY;
	}
}

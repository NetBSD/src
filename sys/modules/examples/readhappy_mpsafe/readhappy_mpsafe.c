/*	$NetBSD: readhappy_mpsafe.c,v 1.1 2018/04/20 00:06:45 kamil Exp $    */

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
__KERNEL_RCSID(0, "$NetBSD: readhappy_mpsafe.c,v 1.1 2018/04/20 00:06:45 kamil Exp $");

#include <sys/param.h>
#include <sys/module.h>
#include <sys/condvar.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/mutex.h>

/*
 * This module is a modification of the readhappy module to illustrate
 * how to make a device MPSAFE (Multiprocessor Safe).
 *
 *  1. Supports opening device by multiple processes but allows only one at a time.
 *  2. Supports multiple read() functions but allows only one at a time.
 *  3. Uses mutex for ensuring synchonization.
 *
 * Create a device /dev/happy_mpsafe from which you can read sequential happy numbers.
 *
 * To use this device you need to do:
 *     mknod /dev/happy_mpsafe c 210 0
 *
 * To test whether the device is MPSAFE. Compile and run the test_readhappy file
 * provided.
 */


#define	HAPPY_NUMBER	1

#define	SAD_NUMBER	4

/*
 * kmutex_t variables have been added to the structure to
 * ensure proper synchronization while opened by multiple devices.
 *
 * kcondvar_t conditional variable added. Boolean part added to
 * check whether the device is in use.
 */

struct happy_softc {
	kcondvar_t	cv;
	bool	    inuse_cv;
	unsigned	last;
	kmutex_t	lock;
	kmutex_t	read_lock;
};


static struct happy_softc sc;

static unsigned
dsum(unsigned n)
{
	unsigned sum, x;

	for (sum = 0; n; n /= 10) {
		x = n % 10;
		sum += x * x;
	}
	return sum;
}

static int
check_happy(unsigned n)
{
	unsigned total;

	KASSERT(mutex_owned(&sc.read_lock));

	for (;;) {
		total = dsum(n);

		if (total == HAPPY_NUMBER)
			return 1;
		if (total == SAD_NUMBER)
			return 0;

		n = total;
	}
}

dev_type_open(happy_open);
dev_type_close(happy_close);
dev_type_read(happy_read);

/*
 *  Notice that the .d_flag has a additional D_MPSAFE flag to
 *  tag is as a multiprocessor safe device.
 */

static struct cdevsw happy_cdevsw = {
	.d_open = happy_open,
	.d_close = happy_close,
	.d_read = happy_read,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER | D_MPSAFE,
};

/*
 * happy_open : used to open the device for read. mutex_enter and mutex_exit:
 * to lock the critical section and allow only a single process to open the
 * device at a time.
 */
int
happy_open(dev_t self __unused, int flag __unused, int mode __unused,
	   struct lwp *l __unused)
{
	int error;

	error = 0;

	mutex_enter(&sc.lock);
	while (sc.inuse_cv == true) {
		error = cv_wait_sig(&sc.cv, &sc.lock);
		if (error)
			break;
	}
	if (!error) {
		sc.inuse_cv = true;
		sc.last = 0;
	}
	mutex_exit(&sc.lock);

	return 0;
}

/*
 * happy_close allows only a single process to close the device at a time.
 * It uses mutex_enter and mutex_exit for the same.
 */
int
happy_close(dev_t self __unused, int flag __unused, int mode __unused,
	    struct lwp *l __unused)
{

	mutex_enter(&sc.lock);
	sc.inuse_cv = false;
	cv_signal(&sc.cv);
	mutex_exit(&sc.lock);

	return 0;
}

/*
 * happy_read allows only a single file descriptor to read at a point of time
 * it uses mutex_enter and mutex_exit: to lock the critical section and allow
 * only a single process to open the device at a time.
 */
int
happy_read(dev_t self __unused, struct uio *uio, int flags __unused)
{
	char line[80];
	int error, len;

	mutex_enter(&sc.read_lock);

	while (check_happy(++sc.last) == 0)
		continue;

	len = snprintf(line, sizeof(line), "%u\n", sc.last);

	if (uio->uio_resid < len) {
		--sc.last;
		error = EINVAL;
		goto fin;
	}

	error = uiomove(line, len, uio);

fin:
	mutex_exit(&sc.read_lock);

	return error;
}

MODULE(MODULE_CLASS_MISC, happy_mpsafe, NULL);

/*
 * Initializing mutex and conditional variables for read() and open().
 * when the module is being loaded.
 */
static int
happy_mpsafe_modcmd(modcmd_t cmd, void *arg __unused)
{
	int cmajor = 210, bmajor = -1;

	switch (cmd) {
	case MODULE_CMD_INIT:
		if (devsw_attach("happy_mpsafe", NULL, &bmajor, &happy_cdevsw,
				 &cmajor))
			return ENXIO;

		mutex_init(&sc.lock, MUTEX_DEFAULT, IPL_NONE);
		mutex_init(&sc.read_lock, MUTEX_DEFAULT, IPL_NONE);
		cv_init(&sc.cv, "example conditional variable");
		return 0;
	case MODULE_CMD_FINI:
		mutex_destroy(&sc.lock);
		mutex_destroy(&sc.read_lock);
		cv_destroy(&sc.cv);
		devsw_detach(NULL, &happy_cdevsw);
		return 0;
	default:
		return ENOTTY;
	}
}

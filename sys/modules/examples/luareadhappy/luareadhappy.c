/*	$NetBSD: luareadhappy.c,v 1.1.16.2 2017/12/03 11:38:53 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: luareadhappy.c,v 1.1.16.2 2017/12/03 11:38:53 jdolecek Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/lua.h>
#include <sys/module.h>
#include <lua.h>

/*
 * Create a device /dev/happy from which you can read sequential
 * happy numbers.
 *
 * To use this device you need to do:
 *     mknod /dev/happy c 210 0
 *
 * Commentary:
 * A happy number is a number defined by the following process: Starting with
 * any positive integer, replace the number by the sum of the squares of its
 * digits, and repeat the process until the number equals 1 (where it will
 * stay), or it loops endlessly in a cycle which does not include 1. Those
 * numbers for which this process ends in 1 are happy numbers, while those that
 * do not end in 1 are unhappy numbers (or sad numbers).
 *
 * For more information on happy numbers, and the algorithms, see
 *	http://en.wikipedia.org/wiki/Happy_number 
 *
 * The happy number generator is here only to have something that the user
 * can read from our device.  Any other arbitrary data generator could
 * have been used.  The algorithm is not critical to the implementation
 * of the module.
 */

dev_type_open(happy_open);
dev_type_close(happy_close);
dev_type_read(happy_read);

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
	.d_flag = D_OTHER
};


struct happy_softc {
	int		 refcnt;
	unsigned	 last;
	klua_State	*kL;
};

static struct happy_softc sc;

/* Function that calls a Lua routine and returns whether a number is happy */
static int
check_happy(unsigned n)
{
	int rv;

	klua_lock(sc.kL);
	lua_getglobal(sc.kL->L, "is_happy");

        if (!lua_isfunction(sc.kL->L, -1)) {
		lua_pop(sc.kL->L, 1);
		klua_unlock(sc.kL);
		return -1;
        }

        lua_pushnumber(sc.kL->L, n);
        if (lua_pcall(sc.kL->L, 1 /* args */, 1 /* res */, 0) != 0) {
		lua_pop(sc.kL->L, 2);
		klua_unlock(sc.kL);
		return -1;
        }

        if (!lua_isnumber(sc.kL->L, -1)) {
		lua_pop(sc.kL->L, 1);
		klua_unlock(sc.kL);
		return -1;
        }

        rv = lua_tointeger(sc.kL->L, -1);

        lua_pop(sc.kL->L, 1);
        klua_unlock(sc.kL);

	/* Consistency check */
	if (rv != 0 && rv != 1)
		rv = -1;

	return rv;
}

int
happy_open(dev_t self __unused, int flag __unused, int mode __unused,
           struct lwp *l __unused)
{
	if (sc.refcnt > 0)
		return EBUSY;

	sc.last = 0;
	++sc.refcnt;

	return 0;
}

int
happy_close(dev_t self __unused, int flag __unused, int mode __unused,
            struct lwp *l __unused)
{
	--sc.refcnt;

	return 0;
}

int
happy_read(dev_t self __unused, struct uio *uio, int flags __unused)
{
	int rv;
	char line[80];

	/* Get next happy number */
	while ((rv = check_happy(++sc.last)) == 0)
		continue;

	/* Something went wrong */
	if (rv == -1)
		return ECANCELED;

	/* Print it into line[] with trailing \n */
	int len = snprintf(line, sizeof(line), "%u\n", sc.last);

	/* Is there room? */
	if (uio->uio_resid < len) {
		--sc.last; /* Step back */
		return EINVAL;
	}

	/* Send it to User-Space */
	int e;
	if ((e = uiomove(line, len, uio)))
		return e;

	return 0;
}

MODULE(MODULE_CLASS_MISC, happy, "lua");

static int
happy_modcmd(modcmd_t cmd, void *arg __unused)
{
	/* The major should be verified and changed if needed to avoid
	 * conflicts with other devices. */
	int cmajor = 210, bmajor = -1;

	switch (cmd) {
	case MODULE_CMD_INIT:
		if (devsw_attach("happy", NULL, &bmajor, &happy_cdevsw,
		                 &cmajor))
			return ENXIO;
		if ((sc.kL = kluaL_newstate("happy",
		                            "Example Happy Number calculator",
		                            IPL_NONE)) == NULL) {
			devsw_detach(NULL, &happy_cdevsw);
			return ENXIO;
		}
		return 0;
	case MODULE_CMD_FINI:
		if (sc.refcnt > 0)
			return EBUSY;

		klua_close(sc.kL);

		devsw_detach(NULL, &happy_cdevsw);
		return 0;
	default:
		return ENOTTY;
	}
}

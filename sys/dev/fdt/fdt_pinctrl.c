/* $NetBSD: fdt_pinctrl.c,v 1.3 2016/10/11 13:04:57 maxv Exp $ */

/*-
 * Copyright (c) 2015 Martin Fouts
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fdt_pinctrl.c,v 1.3 2016/10/11 13:04:57 maxv Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

struct fdtbus_pinctrl_controller {
	int pc_phandle;
	void *pc_cookie;
	const struct fdtbus_pinctrl_controller_func *pc_funcs;

	struct fdtbus_pinctrl_controller *pc_next;
};

static struct fdtbus_pinctrl_controller *fdtbus_pc = NULL;

int
fdtbus_register_pinctrl_config(void *cookie, int phandle,
    const struct fdtbus_pinctrl_controller_func *funcs)
{
	struct fdtbus_pinctrl_controller *pc;

	pc = kmem_alloc(sizeof(*pc), KM_SLEEP);
	pc->pc_cookie = cookie;
	pc->pc_phandle = phandle;
	pc->pc_funcs = funcs;

	pc->pc_next = fdtbus_pc;
	fdtbus_pc = pc;

	return 0;
}

static struct fdtbus_pinctrl_controller *
fdtbus_pinctrl_lookup(int phandle)
{
	struct fdtbus_pinctrl_controller *pc;

	for (pc = fdtbus_pc; pc; pc = pc->pc_next)
		if (pc->pc_phandle == phandle)
			return pc;

	return NULL;
}

int
fdtbus_pinctrl_set_config_index(int phandle, u_int index)
{
	char buf[80];
	int len, handle;
	struct fdtbus_pinctrl_controller *pc;

	snprintf(buf, 80, "pinctrl-%d", index);

	len = OF_getprop(phandle, buf, (char *)&handle,
                        sizeof(handle));
	if (len != sizeof(int)) {
		printf("%s: couldn't get %s.\n", __func__, buf);
               return -1;
       }

	handle = fdtbus_get_phandle_from_native(be32toh(handle));

	pc = fdtbus_pinctrl_lookup(handle);
	if (!pc) {
		printf("%s: Couldn't get handle %d for %s\n", __func__, handle,
		       buf);
		return -1;
	}

	return pc->pc_funcs->set_config(pc->pc_cookie);
}

int
fdtbus_pinctrl_set_config(int phandle, const char *cfgname)
{
	int index = 0;
	int len;
	char *result;
	char *next;

	len = OF_getproplen(phandle, "pinctrl-names");
	if (len <= 0)
		return -1;
	result = kmem_zalloc(len, KM_SLEEP);
	OF_getprop(phandle, "pinctrl-names", result, len);

	next = result;
	while (next - result < len) {
		if (!strcmp(next, cfgname)) {
			kmem_free(result, len);
			return fdtbus_pinctrl_set_config_index(phandle, index);
		}
		index++;
		while (*next)
			next++;
		next++;
	}

	kmem_free(result, len);
	return -1;
}

/* $NetBSD: panic.c,v 1.1 2011/02/18 01:07:20 jmcneill Exp $ */

/*
 * Copyright (c) 2011 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: panic.c,v 1.1 2011/02/18 01:07:20 jmcneill Exp $");

#include <sys/module.h>

MODULE(MODULE_CLASS_MISC, panic, NULL);

static void
panic_dopanic(void)
{
	/* just call panic */
	panic("oops");
}

static void
panic_donullptr(void)
{
	/* null ptr dereference */
	*(int *)NULL = 1;
}

static const struct {
	const char *name;
	void (*func)(void);
} panic_howto[] = {
	{ "panic",	panic_dopanic },
	{ "nullptr",	panic_donullptr },
};

static int
panic_modcmd(modcmd_t cmd, void *opaque)
{
	if (cmd == MODULE_CMD_INIT) {
		prop_dictionary_t props = opaque;
		const char *how = NULL;
		unsigned int i;

		if (props)
			prop_dictionary_get_cstring_nocopy(props, "how", &how);
		if (how == NULL)
			how = "panic";

		for (i = 0; i < __arraycount(panic_howto); i++) {
			if (strcmp(how, panic_howto[i].name) == 0) {
				panic_howto[i].func();
				break;
			}
		}
		if (i == __arraycount(panic_howto))
			printf("%s: no how '%s'\n", __func__, how);
		else
			printf("%s: how '%s' didn't panic?\n", __func__, how);

		return EINVAL;
	}

	return ENOTTY;
}

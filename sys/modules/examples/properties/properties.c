/*	$NetBSD: properties.c,v 1.1.20.2 2017/12/03 11:38:53 jdolecek Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: properties.c,v 1.1.20.2 2017/12/03 11:38:53 jdolecek Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/module.h>

MODULE(MODULE_CLASS_MISC, properties, NULL);

static void
handle_props(prop_dictionary_t props)
{
	const char *msg;
	prop_string_t str;

	if (props != NULL) {
		str = prop_dictionary_get(props, "msg");
	} else {
		printf("No property dictionary was provided.\n");
		str = NULL;
	}
	if (str == NULL)
		printf("The 'msg' property was not given.\n");
	else if (prop_object_type(str) != PROP_TYPE_STRING)
		printf("The 'msg' property is not a string.\n");
	else {
		msg = prop_string_cstring_nocopy(str);
		if (msg == NULL)
			printf("Failed to process the 'msg' property.\n");
		else
			printf("The 'msg' property is: %s\n", msg);
	}
}

static int
properties_modcmd(modcmd_t cmd, void *arg)
{
	switch (cmd) {
	case MODULE_CMD_INIT:
		handle_props(arg);
		return 0;
	case MODULE_CMD_FINI:
		return 0;
	default:
		return ENOTTY;
	}
}

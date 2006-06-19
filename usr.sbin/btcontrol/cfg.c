/*	$NetBSD: cfg.c,v 1.1 2006/06/19 15:44:56 gdamore Exp $	*/

/*-
 * Copyright (c) 2006 Itronix Inc.
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
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: cfg.c,v 1.1 2006/06/19 15:44:56 gdamore Exp $");

#include <bluetooth.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "btcontrol.h"

int
cfg_print(bdaddr_t *laddr, bdaddr_t *raddr, int argc, char **argv)
{
	bt_cfgentry_t *cfg;
	bt_handle_t handle;

	handle = bt_openconfig(config_file);
	if (handle == NULL)
		err(EXIT_FAILURE, "Can't open config file");

	cfg = bt_getconfig(handle, raddr);
	if (cfg == NULL)
		errx(EXIT_FAILURE, "No config entry found");

	bt_printconfig(stdout, cfg);

	bt_freeconfig(cfg);
	bt_closeconfig(handle);

	return EXIT_SUCCESS;
}

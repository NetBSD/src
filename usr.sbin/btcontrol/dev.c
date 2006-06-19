/*	$NetBSD: dev.c,v 1.1 2006/06/19 15:44:56 gdamore Exp $	*/

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
__RCSID("$NetBSD: dev.c,v 1.1 2006/06/19 15:44:56 gdamore Exp $");

#include <sys/ioctl.h>
#include <bluetooth.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dev/bluetooth/btdev.h>

#include "btcontrol.h"

int
dev_attach(bdaddr_t *laddr, bdaddr_t *raddr, int argc, char **argv)
{
	struct btdev_attach_args	 bda;
	bt_cfgentry_t			*cfg;
	bt_handle_t			 handle;
	int				 fd, ch;

	handle = bt_openconfig(config_file);
	if (handle == NULL)
		err(EXIT_FAILURE, "Could not open config file");

	cfg = bt_getconfig(handle, raddr);
	if (cfg == NULL)
		errx(EXIT_FAILURE, "Config entry not found");

	memset(&bda, 0, sizeof(bda));
	bdaddr_copy(&bda.bd_laddr, laddr);
	bdaddr_copy(&bda.bd_raddr, raddr);
	bda.bd_type = cfg->type;

	switch (cfg->type) {
	case BTDEV_HID:
		while ((ch = getopt(argc, argv, "C")) != -1) {
			switch (ch) {
			case 'C':
				bda.bd_hid.hid_flags |= BTHID_CONNECT;
				break;

			default:
				errx(EXIT_FAILURE, "Unknown option");
			}
		}

		if (cfg->control_psm == 0)
			bda.bd_hid.hid_ctl = L2CAP_PSM_HID_CNTL;
		else
			bda.bd_hid.hid_ctl = cfg->control_psm;

		if (cfg->interrupt_psm == 0)
			bda.bd_hid.hid_ctl = L2CAP_PSM_HID_INTR;
		else
			bda.bd_hid.hid_int = cfg->interrupt_psm;

		bda.bd_hid.hid_desc = cfg->hid_descriptor;
		bda.bd_hid.hid_dlen = cfg->hid_length;

		if (!cfg->reconnect_initiate)
			bda.bd_hid.hid_flags |= BTHID_INITIATE;

		break;

	case BTDEV_HSET:
		while ((ch = getopt(argc, argv, "m:")) != -1) {
			switch (ch) {
			case 'm':
				bda.bd_hset.hset_mtu = atoi(optarg);
				break;

			default:
				errx(EXIT_FAILURE, "Unknown option");
			}
		}

		bda.bd_hset.hset_channel = cfg->control_channel;
		break;

	default:
		errx(EXIT_FAILURE, "Unknown Device type");
	}

	bt_freeconfig(cfg);
	bt_closeconfig(handle);

	fd = open(control_file, O_WRONLY, 0);
	if (fd < 0)
		err(EXIT_FAILURE, "%s", control_file);

	if (ioctl(fd, BTDEV_ATTACH, &bda) < 0)
		err(EXIT_FAILURE, "BTDEV_ATTACH");

	close(fd);

	return EXIT_SUCCESS;
}

int
dev_detach(bdaddr_t *laddr, bdaddr_t *raddr, int argc, char **argv)
{
	int fd;

	fd = open(control_file, O_WRONLY, 0);
	if (fd < 0)
		err(EXIT_FAILURE, "%s", control_file);

	if (ioctl(fd, BTDEV_DETACH, raddr) < 0)
		err(EXIT_FAILURE, "BTDEV_DETACH");

	close(fd);

	return EXIT_SUCCESS;
}

/*	$NetBSD: config.c,v 1.1 2006/06/19 15:44:56 gdamore Exp $	*/

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
__RCSID("$NetBSD: config.c,v 1.1 2006/06/19 15:44:56 gdamore Exp $");

#include <sys/time.h>
#include <bluetooth.h>
#include <errno.h>
#include <event.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "bthcid.h"

static	bt_cfgentry_t	*cfg = NULL;

uint8_t *
lookup_key(bdaddr_t *laddr, bdaddr_t *raddr)
{
	bt_handle_t handle;

	if (cfg != NULL) {
		bt_freeconfig(cfg);
		cfg = NULL;
	}

	handle = bt_openconfig(key_file);
	if (handle == NULL)
		return NULL;

	cfg = bt_getconfig(handle, raddr);
	bt_closeconfig(handle);

	if (cfg == NULL) {
		handle = bt_openconfig(config_file);
		if (handle == NULL)
			return NULL;

		cfg = bt_getconfig(handle, raddr);
		bt_closeconfig(handle);
	}

	if (cfg != NULL)
		return cfg->key;

	return NULL;
}

uint8_t *
lookup_pin(bdaddr_t *laddr, bdaddr_t *raddr)
{
	bt_handle_t handle;

	if (cfg != NULL) {
		bt_freeconfig(cfg);
		cfg = NULL;
	}

	handle = bt_openconfig(config_file);
	if (handle != NULL) {
		cfg = bt_getconfig(handle, raddr);
		bt_closeconfig(handle);

		if (cfg != NULL && cfg->pin != NULL)
			return cfg->pin;
	}

	return lookup_item(laddr, raddr);
}

void
save_key(bdaddr_t *laddr, bdaddr_t *raddr, uint8_t *key)
{
	static const char hex[] = "0123456789abcdef";
	char buffer[100], keybuf[HCI_KEY_SIZE * 2 + 1];
	int fd, n, i;

	fd = open(key_file, O_WRONLY|O_APPEND|O_CREAT|O_EXLOCK, 0600);
	if (fd < 0) {
		syslog(LOG_ERR, "Cannot open keyfile %s. %s (%d)",
				key_file, strerror(errno), errno);

		return;
	}

	for (n = i = 0 ; i < HCI_KEY_SIZE ; i++) {
		keybuf[n++] = hex[(key[i] >> 4) & 0xf];
		keybuf[n++] = hex[key[i] & 0xf];
	}
	keybuf[n] = '\0';

	n = snprintf(buffer, sizeof(buffer),
		"\ndevice {\n"
		"\tbdaddr  %s;\n"
		"\tkey     0x%s;\n"
		"}\n",
		bt_ntoa(raddr, NULL),
		keybuf);

	write(fd, buffer, (size_t)n);
	close(fd);
}

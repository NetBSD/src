/*	$NetBSD: config.c,v 1.4.20.1 2009/05/13 19:20:19 jym Exp $	*/

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
__RCSID("$NetBSD: config.c,v 1.4.20.1 2009/05/13 19:20:19 jym Exp $");

#include <sys/time.h>
#include <prop/proplib.h>
#include <bluetooth.h>
#include <errno.h>
#include <event.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "bthcid.h"

static const char *key_file = "/var/db/bthcid.keys";
static const char *new_key_file = "/var/db/bthcid.keys.new";

static prop_dictionary_t
load_keys(void)
{
	prop_dictionary_t dict;
	char *xml;
	off_t len;
	int fd;

	fd = open(key_file, O_RDONLY, 0);
	if (fd < 0)
		return NULL;

	len = lseek(fd, 0, SEEK_END);
	if (len == 0) {
		close(fd);
		return NULL;
	}

	xml = malloc(len);
	if (xml == NULL) {
		close(fd);
		return NULL;
	}

	(void)lseek(fd, 0, SEEK_SET);
	if (read(fd, xml, len) != len) {
		free(xml);
		close(fd);
		return NULL;
	}

	dict = prop_dictionary_internalize(xml);
	free(xml);
	close(fd);
	return dict;
}

/*
 * Look up key in keys file. We store a dictionary for each
 * remote address, and inside that we have a data object for
 * each local address containing the key.
 */
uint8_t *
lookup_key(bdaddr_t *laddr, bdaddr_t *raddr)
{
	static uint8_t key[HCI_KEY_SIZE];
	prop_dictionary_t cfg;
	prop_object_t obj;

	cfg = load_keys();
	if (cfg == NULL)
		return NULL;

	obj = prop_dictionary_get(cfg, bt_ntoa(laddr, NULL));
	if (obj == NULL || prop_object_type(obj) != PROP_TYPE_DICTIONARY) {
		prop_object_release(cfg);
		return NULL;
	}

	obj = prop_dictionary_get(obj, bt_ntoa(raddr, NULL));
	if (obj == NULL || prop_object_type(obj) != PROP_TYPE_DATA
	    || prop_data_size(obj) != sizeof(key)) {
		prop_object_release(cfg);
		return NULL;
	}

	memcpy(key, prop_data_data_nocopy(obj), sizeof(key));
	prop_object_release(cfg);
	return key;
}

void
save_key(bdaddr_t *laddr, bdaddr_t *raddr, uint8_t *key)
{
	prop_dictionary_t cfg, dev;
	prop_data_t dat;
	char *xml;
	int fd;
	size_t len;

	cfg = load_keys();
	if (cfg == NULL) {
		cfg = prop_dictionary_create();
		if (cfg == NULL) {
			syslog(LOG_ERR, "prop_dictionary_create() failed: %m");
			return;
		}
	}

	dev = prop_dictionary_get(cfg, bt_ntoa(laddr, NULL));
	if (dev == NULL) {
		dev = prop_dictionary_create();
		if (dev == NULL) {
			syslog(LOG_ERR, "prop_dictionary_create() failed: %m");
			prop_object_release(cfg);
			return;
		}

		if (!prop_dictionary_set(cfg, bt_ntoa(laddr, NULL), dev)) {
			syslog(LOG_ERR, "prop_dictionary_set() failed: %m");
			prop_object_release(dev);
			prop_object_release(cfg);
			return;
		}

		prop_object_release(dev);
	}

	dat = prop_data_create_data_nocopy(key, HCI_KEY_SIZE);
	if (dat == NULL) {
		syslog(LOG_ERR, "Cannot create data object: %m");
		prop_object_release(cfg);
		return;
	}

	if (!prop_dictionary_set(dev, bt_ntoa(raddr, NULL), dat)) {
		syslog(LOG_ERR, "prop_dictionary_set() failed: %m");
		prop_object_release(dat);
		prop_object_release(cfg);
		return;
	}

	prop_object_release(dat);

	xml = prop_dictionary_externalize(cfg);
	if (xml == NULL) {
		syslog(LOG_ERR, "prop_dictionary_externalize() failed: %m");
		prop_object_release(cfg);
		return;
	}

	prop_object_release(cfg);

	fd = open(new_key_file, O_WRONLY|O_TRUNC|O_CREAT|O_EXLOCK, 0600);
	if (fd < 0) {
		syslog(LOG_ERR, "Cannot open new keyfile %s: %m", key_file);
		free(xml);
		return;
	}

	len = strlen(xml);
	if ((size_t)write(fd, xml, len) != len) {
		syslog(LOG_ERR, "Write of keyfile failed: %m");
		free(xml);
		close(fd);
		unlink(new_key_file);
		return;
	}

	free(xml);
	close(fd);

	if (rename(new_key_file, key_file) < 0) {
		syslog(LOG_ERR, "rename(%s, %s) failed: %m",
			new_key_file, key_file);

		unlink(new_key_file);
	}
}

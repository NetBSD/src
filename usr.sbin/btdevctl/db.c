/*	$NetBSD: db.c,v 1.1.4.1 2007/07/19 16:04:22 liamjfoy Exp $	*/

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Iain Hibbert for Itronix Inc.
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
__RCSID("$NetBSD: db.c,v 1.1.4.1 2007/07/19 16:04:22 liamjfoy Exp $");

#include <bluetooth.h>
#include <err.h>
#include <stdlib.h>

#include <prop/proplib.h>

#include <dev/bluetooth/btdev.h>
#include <dev/bluetooth/bthidev.h>
#include <dev/bluetooth/btsco.h>

#include "btdevctl.h"

#define BTDEVCTL_PLIST		"/var/db/btdevctl.plist"
#define BTDEVCTL_VERSION	2

static prop_dictionary_t db = NULL;
static int db_flush = TRUE;		/* write db on set */

static void db_update0(void);
static void db_update1(void);

/*
 * lookup laddr/raddr/service in database and return dictionary
 */
prop_dictionary_t
db_get(bdaddr_t *laddr, bdaddr_t *raddr, const char *service)
{
	prop_dictionary_t ldev, rdev, dev;
	prop_object_t obj;

	if (db == NULL) {
		db = prop_dictionary_internalize_from_file(BTDEVCTL_PLIST);
		if (db == NULL) {
			db = prop_dictionary_create();
			if (db == NULL)
				err(EXIT_FAILURE, "prop_dictionary_create");

			return NULL;
		} else {
			obj = prop_dictionary_get(db, "btdevctl-version");
			switch(prop_number_integer_value(obj)) {
			case 0: db_update0();
			case 1: db_update1();
			case BTDEVCTL_VERSION:
				break;

			default:
				errx(EXIT_FAILURE, "unknown btdevctl-version");
			}
		}
	}

	ldev = prop_dictionary_get(db, bt_ntoa(laddr, NULL));
	if (prop_object_type(ldev) != PROP_TYPE_DICTIONARY)
		return NULL;

	rdev = prop_dictionary_get(ldev, bt_ntoa(raddr, NULL));
	if (prop_object_type(rdev) != PROP_TYPE_DICTIONARY)
		return NULL;

	dev = prop_dictionary_get(rdev, service);
	if (prop_object_type(dev) != PROP_TYPE_DICTIONARY)
		return NULL;

	return dev;
}

/*
 * store dictionary in database at laddr/raddr/service
 */
int
db_set(prop_dictionary_t dev, bdaddr_t *laddr, bdaddr_t *raddr, const char *service)
{
	prop_dictionary_t ldev, rdev;
	prop_number_t version;

	ldev = prop_dictionary_get(db, bt_ntoa(laddr, NULL));
	if (ldev == NULL) {
		ldev = prop_dictionary_create();
		if (ldev == NULL)
			return 0;

		if (!prop_dictionary_set(db, bt_ntoa(laddr, NULL), ldev))
			return 0;

		prop_object_release(ldev);
	}

	rdev = prop_dictionary_get(ldev, bt_ntoa(raddr, NULL));
	if (rdev == NULL) {
		rdev = prop_dictionary_create();
		if (rdev == NULL)
			return 0;

		if (!prop_dictionary_set(ldev, bt_ntoa(raddr, NULL), rdev))
			return 0;

		prop_object_release(rdev);
	}

	if (!prop_dictionary_set(rdev, service, dev))
		return 0;

	if (db_flush == TRUE) {
		version = prop_number_create_integer(BTDEVCTL_VERSION);
		if (version == NULL)
			err(EXIT_FAILURE, "prop_number_create_integer");

		if (!prop_dictionary_set(db, "btdevctl-version", version))
			err(EXIT_FAILURE, "prop_dictionary_set");

		prop_object_release(version);

		if (!prop_dictionary_externalize_to_file(db, BTDEVCTL_PLIST))
			warn("%s", BTDEVCTL_PLIST);
	}

	return 1;
}

/*
 * update database from version 0. This was a flat file using
 * btdevN as an index, and local-bdaddr and remote-bdaddr stored
 * as data objects. Step through and add them to the new dictionary.
 * We have to generate the service, but only HID, HF and HSET
 * were supported, so thats not too difficult.
 */
static void
db_update0(void)
{
	prop_dictionary_t old, dev;
	prop_dictionary_keysym_t key;
	prop_object_iterator_t iter;
	prop_object_t obj;
	bdaddr_t laddr, raddr;
	char *service;

	db_flush = FALSE;	/* no write on set */
	old = db;

	db = prop_dictionary_create();
	if (db == NULL)
		err(EXIT_FAILURE, "prop_dictionary_create");

	iter = prop_dictionary_iterator(old);
	if (iter == NULL)
		err(EXIT_FAILURE, "prop_dictionary_iterator");

	while ((key = prop_object_iterator_next(iter)) != NULL) {
		dev = prop_dictionary_get_keysym(old, key);
		if (prop_object_type(dev) != PROP_TYPE_DICTIONARY)
			errx(EXIT_FAILURE, "invalid device dictionary");

		obj = prop_dictionary_get(dev, BTDEVladdr);
		if (prop_data_size(obj) != sizeof(laddr))
			errx(EXIT_FAILURE, "invalid %s", BTDEVladdr);

		bdaddr_copy(&laddr, prop_data_data_nocopy(obj));
		prop_dictionary_remove(dev, BTDEVladdr);

		obj = prop_dictionary_get(dev, BTDEVraddr);
		if (prop_data_size(obj) != sizeof(raddr))
			errx(EXIT_FAILURE, "invalid %s", BTDEVraddr);

		bdaddr_copy(&raddr, prop_data_data_nocopy(obj));
		prop_dictionary_remove(dev, BTDEVraddr);

		obj = prop_dictionary_get(dev, BTDEVtype);
		if (prop_string_equals_cstring(obj, "bthidev"))
			service = "HID";
		else if (prop_string_equals_cstring(obj, "btsco")) {
			obj = prop_dictionary_get(dev, BTSCOlisten);
			if (prop_bool_true(obj))
				service = "HF";
			else
				service = "HSET";
		} else
			errx(EXIT_FAILURE, "invalid %s", BTDEVtype);

		if (!db_set(dev, &laddr, &raddr, service))
			err(EXIT_FAILURE, "service store failed");
	}

	prop_object_iterator_release(iter);
	prop_object_release(old);

	db_flush = TRUE;	/* write on set */
}

/*
 * update database from version 1. Link Mode capability was added.
 * By default, we request authentication for HIDs, and encryption
 * is enabled for keyboards.
 */
static void
db_update1(void)
{
	prop_dictionary_t ldev, rdev, srv;
	prop_object_iterator_t iter0, iter1;
	prop_dictionary_keysym_t key;
	prop_object_t obj;
	bdaddr_t bdaddr;

	iter0 = prop_dictionary_iterator(db);
	if (iter0 == NULL)
		err(EXIT_FAILURE, "prop_dictionary_iterator");

	while ((key = prop_object_iterator_next(iter0)) != NULL) {
		ldev = prop_dictionary_get_keysym(db, key);
		if (prop_object_type(ldev) != PROP_TYPE_DICTIONARY
		    || !bt_aton(prop_dictionary_keysym_cstring_nocopy(key), &bdaddr))
			continue;

		iter1 = prop_dictionary_iterator(ldev);
		if (iter1 == NULL)
			err(EXIT_FAILURE, "prop_dictionary_iterator");

		while ((key = prop_object_iterator_next(iter1)) != NULL) {
			rdev = prop_dictionary_get_keysym(ldev, key);
			if (prop_object_type(rdev) != PROP_TYPE_DICTIONARY
			    || !bt_aton(prop_dictionary_keysym_cstring_nocopy(key), &bdaddr))
				continue;

			srv = prop_dictionary_get(rdev, "HID");
			if (prop_object_type(srv) != PROP_TYPE_DICTIONARY)
				continue;

			obj = prop_dictionary_get(srv, BTHIDEVdescriptor);
			if (prop_object_type(obj) != PROP_TYPE_DATA)
				continue;

			obj = prop_string_create_cstring_nocopy(hid_mode(obj));
			if (obj == NULL || !prop_dictionary_set(srv, BTDEVmode, obj))
				err(EXIT_FAILURE, "Cannot set %s", BTDEVmode);

			prop_object_release(obj);
		}

		prop_object_iterator_release(iter1);
	}

	prop_object_iterator_release(iter0);
}

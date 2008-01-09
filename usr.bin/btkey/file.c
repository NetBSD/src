/*	$NetBSD: file.c,v 1.1.4.2 2008/01/09 02:00:33 matt Exp $	*/

/*-
 * Copyright (c) 2007 Iain Hibbert
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: file.c,v 1.1.4.2 2008/01/09 02:00:33 matt Exp $");

#include <sys/stat.h>
#include <prop/proplib.h>

#include <bluetooth.h>
#include <stdbool.h>
#include <string.h>

#include "btkey.h"

static const char *key_file = "/var/db/bthcid.keys";

/*
 * List keys file.
 */
bool
list_file(void)
{
	prop_dictionary_t db, dev;
	prop_object_iterator_t iter;
	prop_dictionary_keysym_t sym;
	prop_object_t dat;
	bdaddr_t bdaddr;
	bool rv = false;

	db = prop_dictionary_internalize_from_file(key_file);
	if (db == NULL)
		return false;

	dev = prop_dictionary_get(db, bt_ntoa(&laddr, NULL));
	if (prop_object_type(dev) != PROP_TYPE_DICTIONARY)
		goto done;

	iter = prop_dictionary_iterator(dev);
	if (iter == NULL)
		goto done;

	while ((sym = prop_object_iterator_next(iter)) != NULL) {
		memset(&bdaddr, 0, sizeof(bdaddr));
		bt_aton(prop_dictionary_keysym_cstring_nocopy(sym), &bdaddr);
		if (bdaddr_any(&bdaddr))
			continue;

		dat = prop_dictionary_get_keysym(dev, sym);
		if (prop_data_size(dat) != HCI_KEY_SIZE)
			continue;

		printf("\n");
		print_addr("bdaddr", &bdaddr);
		print_key("file key", prop_data_data_nocopy(dat));
	}

	prop_object_iterator_release(iter);
	rv = true;

done:
	prop_object_release(db);
	return rv;
}

/*
 * Read from keys file.
 */
bool
read_file(void)
{
	prop_dictionary_t db, dev;
	prop_object_t dat;
	bool rv = false;

	db = prop_dictionary_internalize_from_file(key_file);
	if (db == NULL)
		return false;

	dev = prop_dictionary_get(db, bt_ntoa(&laddr, NULL));
	if (prop_object_type(dev) != PROP_TYPE_DICTIONARY)
		goto done;

	dat = prop_dictionary_get(dev, bt_ntoa(&raddr, NULL));
	if (prop_data_size(dat) != HCI_KEY_SIZE)
		goto done;

	memcpy(key, prop_data_data_nocopy(dat), HCI_KEY_SIZE);
	rv = true;

done:
	prop_object_release(db);
	return rv;
}

/*
 * Write to keys file.
 */
bool
write_file(void)
{
	prop_dictionary_t db, dev;
	prop_data_t dat;
	mode_t mode;
	bool rv = false;

	db = prop_dictionary_internalize_from_file(key_file);
	if (db == NULL) {
		db = prop_dictionary_create();
		if (db == NULL)
			return false;
	}

	dev = prop_dictionary_get(db, bt_ntoa(&laddr, NULL));
	if (dev == NULL) {
		dev = prop_dictionary_create();
		if (dev == NULL)
			goto done;

		rv = prop_dictionary_set(db, bt_ntoa(&laddr, NULL), dev);
		prop_object_release(dev);
		if (rv == false)
			goto done;
	}

	dat = prop_data_create_data_nocopy(key, HCI_KEY_SIZE);
	if (dat == NULL)
		goto done;

	rv = prop_dictionary_set(dev, bt_ntoa(&raddr, NULL), dat);
	prop_object_release(dat);
	if (rv == false)
		goto done;

	mode = umask(S_IRWXG | S_IRWXO);
	rv = prop_dictionary_externalize_to_file(db, key_file);
	umask(mode);

done:
	prop_object_release(db);
	return rv;
}

/*
 * Clear from keys file.
 */
bool
clear_file(void)
{
	prop_dictionary_t db, dev;
	prop_data_t dat;
	mode_t mode;
	bool rv = false;

	db = prop_dictionary_internalize_from_file(key_file);
	if (db == NULL)
		return false;

	dev = prop_dictionary_get(db, bt_ntoa(&laddr, NULL));
	if (dev == NULL)
		goto done;

	dat = prop_dictionary_get(dev, bt_ntoa(&raddr, NULL));
	if (prop_data_size(dat) != HCI_KEY_SIZE)
		goto done;

	prop_dictionary_remove(dev, bt_ntoa(&raddr, NULL));

	mode = umask(S_IRWXG | S_IRWXO);
	rv = prop_dictionary_externalize_to_file(db, key_file);
	umask(mode);

done:
	prop_object_release(db);
	return rv;
}

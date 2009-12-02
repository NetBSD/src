/*	$NetBSD: uuid.c,v 1.1.1.3 2009/12/02 00:26:49 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2006 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "lib.h"
#include "uuid.h"
#include "lvm-wrappers.h"

#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

static const char _c[] =
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!#";

static int _built_inverse;
static char _inverse_c[256];

int lvid_create(union lvid *lvid, struct id *vgid)
{
	memcpy(lvid->id, vgid, sizeof(*lvid->id));
	return id_create(&lvid->id[1]);
}

void uuid_from_num(char *uuid, uint32_t num)
{
	unsigned i;

	for (i = ID_LEN; i; i--) {
		uuid[i - 1] = _c[num % (sizeof(_c) - 1)];
		num /= sizeof(_c) - 1;
	}
}

int lvid_from_lvnum(union lvid *lvid, struct id *vgid, uint32_t lv_num)
{
	int i;

	memcpy(lvid->id, vgid, sizeof(*lvid->id));

	for (i = ID_LEN; i; i--) {
		lvid->id[1].uuid[i - 1] = _c[lv_num % (sizeof(_c) - 1)];
		lv_num /= sizeof(_c) - 1;
	}

	lvid->s[sizeof(lvid->s) - 1] = '\0';

	return 1;
}

int lvnum_from_lvid(union lvid *lvid)
{
	int i, lv_num = 0;
	char *c;

	for (i = 0; i < ID_LEN; i++) {
		lv_num *= sizeof(_c) - 1;
		if ((c = strchr(_c, lvid->id[1].uuid[i])))
			lv_num += (int) (c - _c);
		if (lv_num < 0)
			lv_num = 0;
	}

	return lv_num;
}

int lvid_in_restricted_range(union lvid *lvid)
{
	int i;

	for (i = 0; i < ID_LEN - 3; i++)
		if (lvid->id[1].uuid[i] != '0')
			return 0;

	for (i = ID_LEN - 3; i < ID_LEN; i++)
		if (!isdigit(lvid->id[1].uuid[i]))
			return 0;

	return 1;
}


int id_create(struct id *id)
{
	unsigned i;
	size_t len = sizeof(id->uuid);

	memset(id->uuid, 0, len);
	if (!read_urandom(&id->uuid, len)) {
		return 0;
	}

	/*
	 * Skip out the last 2 chars in randomized creation for LVM1
	 * backwards compatibility.
	 */
	for (i = 0; i < len; i++)
		id->uuid[i] = _c[id->uuid[i] % (sizeof(_c) - 3)];

	return 1;
}

/*
 * The only validity check we have is that
 * the uuid just contains characters from
 * '_c'.  A checksum would have been nice :(
 */
static void _build_inverse(void)
{
	const char *ptr;

	if (_built_inverse)
		return;

	memset(_inverse_c, 0, sizeof(_inverse_c));

	for (ptr = _c; *ptr; ptr++)
		_inverse_c[(int) *ptr] = (char) 0x1;
}

int id_valid(struct id *id)
{
	int i;

	_build_inverse();

	for (i = 0; i < ID_LEN; i++)
		if (!_inverse_c[id->uuid[i]]) {
			log_error("UUID contains invalid character");
			return 0;
		}

	return 1;
}

int id_equal(const struct id *lhs, const struct id *rhs)
{
	return !memcmp(lhs->uuid, rhs->uuid, sizeof(lhs->uuid));
}

#define GROUPS (ID_LEN / 4)

int id_write_format(const struct id *id, char *buffer, size_t size)
{
	int i, tot;

	static unsigned group_size[] = { 6, 4, 4, 4, 4, 4, 6 };

	assert(ID_LEN == 32);

	/* split into groups separated by dashes */
	if (size < (32 + 6 + 1)) {
		log_error("Couldn't write uuid, buffer too small.");
		return 0;
	}

	for (i = 0, tot = 0; i < 7; i++) {
		memcpy(buffer, id->uuid + tot, group_size[i]);
		buffer += group_size[i];
		tot += group_size[i];
		*buffer++ = '-';
	}

	*--buffer = '\0';
	return 1;
}

int id_read_format(struct id *id, const char *buffer)
{
	int out = 0;

	/* just strip out any dashes */
	while (*buffer) {

		if (*buffer == '-') {
			buffer++;
			continue;
		}

		if (out >= ID_LEN) {
			log_error("Too many characters to be uuid.");
			return 0;
		}

		id->uuid[out++] = *buffer++;
	}

	if (out != ID_LEN) {
		log_error("Couldn't read uuid: incorrect number of "
			  "characters.");
		return 0;
	}

	return id_valid(id);
}

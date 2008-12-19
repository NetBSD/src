/*	$NetBSD: uuid.h,v 1.3 2008/12/19 15:24:06 haad Exp $	*/

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

#ifndef _LVM_UUID_H
#define _LVM_UUID_H

#define ID_LEN 32
#define ID_LEN_S "32"

struct id {
	int8_t uuid[ID_LEN];
};

/*
 * Unique logical volume identifier
 * With format1 this is VG uuid + LV uuid + '\0' + padding
 */
union lvid {
	struct id id[2];
	char s[2 * sizeof(struct id) + 1 + 7];
};

int lvid_from_lvnum(union lvid *lvid, struct id *vgid, uint32_t lv_num);
int lvnum_from_lvid(union lvid *lvid);
int lvid_in_restricted_range(union lvid *lvid);

void uuid_from_num(char *uuid, uint32_t num);

int lvid_create(union lvid *lvid, struct id *vgid);
int id_create(struct id *id);
int id_valid(struct id *id);
int id_equal(const struct id *lhs, const struct id *rhs);

/*
 * Fills 'buffer' with a more human readable form
 * of the uuid.
 */
int id_write_format(const struct id *id, char *buffer, size_t size);

/*
 * Reads a formatted uuid.
 */
int id_read_format(struct id *id, const char *buffer);

#endif
/*	$NetBSD: uuid.h,v 1.3 2008/12/19 15:24:06 haad Exp $	*/

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

#ifndef _LVM_UUID_H
#define _LVM_UUID_H

#define ID_LEN 32
#define ID_LEN_S "32"

struct id {
	int8_t uuid[ID_LEN];
};

/*
 * Unique logical volume identifier
 * With format1 this is VG uuid + LV uuid + '\0' + padding
 */
union lvid {
	struct id id[2];
	char s[2 * sizeof(struct id) + 1 + 7];
};

int lvid_from_lvnum(union lvid *lvid, struct id *vgid, uint32_t lv_num);
int lvnum_from_lvid(union lvid *lvid);
int lvid_in_restricted_range(union lvid *lvid);

void uuid_from_num(char *uuid, uint32_t num);

int lvid_create(union lvid *lvid, struct id *vgid);
int id_create(struct id *id);
int id_valid(struct id *id);
int id_equal(const struct id *lhs, const struct id *rhs);

/*
 * Fills 'buffer' with a more human readable form
 * of the uuid.
 */
int id_write_format(const struct id *id, char *buffer, size_t size);

/*
 * Reads a formatted uuid.
 */
int id_read_format(struct id *id, const char *buffer);

#endif

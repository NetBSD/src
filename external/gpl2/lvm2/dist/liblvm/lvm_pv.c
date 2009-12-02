/*	$NetBSD: lvm_pv.c,v 1.1.1.1 2009/12/02 00:26:15 haad Exp $	*/

/*
 * Copyright (C) 2008,2009 Red Hat, Inc. All rights reserved.
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
#include "lvm2app.h"
#include "metadata-exported.h"
#include "lvm-string.h"

char *lvm_pv_get_uuid(const pv_t pv)
{
	char uuid[64] __attribute((aligned(8)));

	if (!id_write_format(&pv->id, uuid, sizeof(uuid))) {
		log_error("Internal error converting uuid");
		return NULL;
	}
	return strndup((const char *)uuid, 64);
}

char *lvm_pv_get_name(const pv_t pv)
{
	char *name;

	name = dm_malloc(NAME_LEN + 1);
	strncpy(name, (const char *)pv_dev_name(pv), NAME_LEN);
	name[NAME_LEN] = '\0';
	return name;
}

uint64_t lvm_pv_get_mda_count(const pv_t pv)
{
	return (uint64_t) pv_mda_count(pv);
}

int lvm_pv_resize(const pv_t pv, uint64_t new_size)
{
	/* FIXME: add pv resize code here */
	log_error("NOT IMPLEMENTED YET");
	return -1;
}

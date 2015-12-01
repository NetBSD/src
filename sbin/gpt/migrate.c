/*-
 * Copyright (c) 2002 Marcel Moolenaar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#ifdef __FBSDID
__FBSDID("$FreeBSD: src/sbin/gpt/migrate.c,v 1.16 2005/09/01 02:42:52 marcel Exp $");
#endif
#ifdef __RCSID
__RCSID("$NetBSD: migrate.c,v 1.25 2015/12/01 19:25:24 christos Exp $");
#endif

#include <sys/types.h>
#include <sys/param.h>
#ifdef HAVE_NBTOOL_CONFIG_H
#include <nbinclude/sys/bootblock.h>
#include <nbinclude/sys/disklabel.h>
#else
#include <sys/bootblock.h>
#include <sys/disklabel.h>
#endif

#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "map.h"
#include "gpt.h"
#include "gpt_private.h"

/*
 * Allow compilation on platforms that do not have a BSD label.
 * The values are valid for amd64, i386 and ia64 disklabels.
 * XXX: use disklabel_params from disklabel.c
 */
#ifndef LABELOFFSET
#define	LABELOFFSET	0
#endif
#ifndef LABELSECTOR
#define	LABELSECTOR	1
#endif
#ifndef RAW_PART
#define	RAW_PART	3
#endif

/* FreeBSD filesystem types that don't match corresponding NetBSD types */
#define	FREEBSD_FS_VINUM	14
#define	FREEBSD_FS_ZFS		27

static int force;
static int slice;
static u_int parts;

static int cmd_migrate(gpt_t, int, char *[]);

static const char *migratehelp[] = {
    "[-fs] [-p <partitions>]",
};

struct gpt_cmd c_migrate = {
	"migrate",
	cmd_migrate,
	migratehelp, __arraycount(migratehelp),
	0,
};

#define usage() gpt_usage(NULL, &c_migrate)

static struct gpt_ent *
migrate_disklabel(gpt_t gpt, off_t start, struct gpt_ent *ent)
{
	char *buf;
	struct disklabel *dl;
	off_t ofs, rawofs;
	int i;

	buf = gpt_read(gpt, start + LABELSECTOR, 1);
	if (buf == NULL) {
		gpt_warn(gpt, "Error reading label");
		return NULL;
	}
	dl = (void*)(buf + LABELOFFSET);

	if (le32toh(dl->d_magic) != DISKMAGIC ||
	    le32toh(dl->d_magic2) != DISKMAGIC) {
		gpt_warnx(gpt, "FreeBSD slice without disklabel");
		free(buf);
		return (ent);
	}

	rawofs = le32toh(dl->d_partitions[RAW_PART].p_offset) *
	    le32toh(dl->d_secsize);
	for (i = 0; i < le16toh(dl->d_npartitions); i++) {
		if (dl->d_partitions[i].p_fstype == FS_UNUSED)
			continue;
		ofs = le32toh(dl->d_partitions[i].p_offset) *
		    le32toh(dl->d_secsize);
		if (ofs < rawofs)
			rawofs = 0;
	}
	rawofs /= gpt->secsz;

	for (i = 0; i < le16toh(dl->d_npartitions); i++) {
		switch (dl->d_partitions[i].p_fstype) {
		case FS_UNUSED:
			continue;
		case FS_SWAP: {
			gpt_uuid_create(GPT_TYPE_FREEBSD_SWAP, ent->ent_type,
			    ent->ent_name, sizeof(ent->ent_name));
			break;
		}
		case FS_BSDFFS: {
			gpt_uuid_create(GPT_TYPE_FREEBSD_UFS, ent->ent_type,
			    ent->ent_name, sizeof(ent->ent_name));
			break;
		}
		case FREEBSD_FS_VINUM: {
			gpt_uuid_create(GPT_TYPE_FREEBSD_VINUM, ent->ent_type,
			    ent->ent_name, sizeof(ent->ent_name));
			break;
		}
		case FREEBSD_FS_ZFS: {
			gpt_uuid_create(GPT_TYPE_FREEBSD_ZFS, ent->ent_type,
			    ent->ent_name, sizeof(ent->ent_name));
			break;
		}
		default:
			gpt_warnx(gpt, "Unknown FreeBSD partition (%d)",
			    dl->d_partitions[i].p_fstype);
			continue;
		}

		ofs = (le32toh(dl->d_partitions[i].p_offset) *
		    le32toh(dl->d_secsize)) / gpt->secsz;
		ofs = (ofs > 0) ? ofs - rawofs : 0;
		ent->ent_lba_start = htole64(start + ofs);
		ent->ent_lba_end = htole64(start + ofs +
		    le32toh(dl->d_partitions[i].p_size) - 1LL);
		ent++;
	}

	free(buf);
	return ent;
}

static struct gpt_ent*
migrate_netbsd_disklabel(gpt_t gpt, off_t start, struct gpt_ent *ent)
{
	char *buf;
	struct disklabel *dl;
	off_t ofs, rawofs;
	int i;

	buf = gpt_read(gpt, start + LABELSECTOR, 1);
	if (buf == NULL) {
		gpt_warn(gpt, "Error reading label");
		return NULL;
	}
	dl = (void*)(buf + LABELOFFSET);

	if (le32toh(dl->d_magic) != DISKMAGIC ||
	    le32toh(dl->d_magic2) != DISKMAGIC) {
		gpt_warnx(gpt, "NetBSD slice without disklabel");
		free(buf);
		return ent;
	}

	rawofs = le32toh(dl->d_partitions[RAW_PART].p_offset) *
	    le32toh(dl->d_secsize);
	for (i = 0; i < le16toh(dl->d_npartitions); i++) {
		if (dl->d_partitions[i].p_fstype == FS_UNUSED)
			continue;
		ofs = le32toh(dl->d_partitions[i].p_offset) *
		    le32toh(dl->d_secsize);
		if (ofs < rawofs)
			rawofs = 0;
	}
	rawofs /= gpt->secsz;

	for (i = 0; i < le16toh(dl->d_npartitions); i++) {
		switch (dl->d_partitions[i].p_fstype) {
		case FS_UNUSED:
			continue;
		case FS_SWAP: {
			gpt_uuid_create(GPT_TYPE_NETBSD_SWAP, ent->ent_type,
			    ent->ent_name, sizeof(ent->ent_name));
			break;
		}
		case FS_BSDFFS: {
			gpt_uuid_create(GPT_TYPE_NETBSD_FFS, ent->ent_type,
			    ent->ent_name, sizeof(ent->ent_name));
			break;
		}
		case FS_BSDLFS: {
			gpt_uuid_create(GPT_TYPE_NETBSD_LFS, ent->ent_type,
			    ent->ent_name, sizeof(ent->ent_name));
			break;
		}
		case FS_RAID: {
			gpt_uuid_create(GPT_TYPE_NETBSD_RAIDFRAME, ent->ent_type,
			    ent->ent_name, sizeof(ent->ent_name));
			break;
		}
		case FS_CCD: {
			gpt_uuid_create(GPT_TYPE_NETBSD_CCD, ent->ent_type,
			    ent->ent_name, sizeof(ent->ent_name));
			break;
		}
		case FS_CGD: {
			gpt_uuid_create(GPT_TYPE_NETBSD_CGD, ent->ent_type,
			    ent->ent_name, sizeof(ent->ent_name));
			break;
		}
		default:
			gpt_warnx(gpt, "Unknown NetBSD partition (%d)",
			    dl->d_partitions[i].p_fstype);
			continue;
		}

		ofs = (le32toh(dl->d_partitions[i].p_offset) *
		    le32toh(dl->d_secsize)) / gpt->secsz;
		ofs = (ofs > 0) ? ofs - rawofs : 0;
		ent->ent_lba_start = htole64(ofs);
		ent->ent_lba_end = htole64(ofs +
		    le32toh(dl->d_partitions[i].p_size) - 1LL);
		ent++;
	}

	free(buf);
	return ent;
}

static int
migrate(gpt_t gpt)
{
	off_t last = gpt_last(gpt);
	map_t map;
	struct gpt_ent *ent;
	struct mbr *mbr;
	uint32_t start, size;
	unsigned int i;

	map = map_find(gpt, MAP_TYPE_MBR);
	if (map == NULL || map->map_start != 0) {
		gpt_warnx(gpt, "No partitions to convert");
		return -1;
	}

	mbr = map->map_data;

	if (gpt_create(gpt, last, parts, 0) == -1)
		return -1;

	ent = gpt->tbl->map_data;

	/* Mirror partitions. */
	for (i = 0; i < 4; i++) {
		start = le16toh(mbr->mbr_part[i].part_start_hi);
		start = (start << 16) + le16toh(mbr->mbr_part[i].part_start_lo);
		size = le16toh(mbr->mbr_part[i].part_size_hi);
		size = (size << 16) + le16toh(mbr->mbr_part[i].part_size_lo);

		switch (mbr->mbr_part[i].part_typ) {
		case MBR_PTYPE_UNUSED:
			continue;
		case MBR_PTYPE_386BSD: {	/* FreeBSD */
			if (slice) {
				gpt_uuid_create(GPT_TYPE_FREEBSD,
				    ent->ent_type, ent->ent_name,
				    sizeof(ent->ent_name));
				ent->ent_lba_start = htole64((uint64_t)start);
				ent->ent_lba_end = htole64(start + size - 1LL);
				ent++;
			} else
				ent = migrate_disklabel(gpt, start, ent);
			break;
		}
		case MBR_PTYPE_NETBSD:
			ent = migrate_netbsd_disklabel(gpt, start, ent);
			break;
		case MBR_PTYPE_EFI: {
			gpt_uuid_create(GPT_TYPE_EFI,
			    ent->ent_type, ent->ent_name,
			    sizeof(ent->ent_name));
			ent->ent_lba_start = htole64((uint64_t)start);
			ent->ent_lba_end = htole64(start + size - 1LL);
			ent++;
			break;
		}
		default:
			if (!force) {
				gpt_warnx(gpt, "unknown partition type (%d)",
				    mbr->mbr_part[i].part_typ);
				return -1;
			}
			break;
		}
	}

	if (gpt_write_primary(gpt) == -1)
		return -1;

	if (gpt_write_backup(gpt) == -1)
		return -1;

	/*
	 * Turn the MBR into a Protective MBR.
	 */
	memset(mbr->mbr_part, 0, sizeof(mbr->mbr_part));
	gpt_create_pmbr_part(mbr->mbr_part, last);
	if (gpt_write(gpt, map) == -1) {
		gpt_warn(gpt, "Cant write PMBR");
		return -1;
	}
	return 0;
}

static int
cmd_migrate(gpt_t gpt, int argc, char *argv[])
{
	int ch;

	parts = 128;

	/* Get the migrate options */
	while ((ch = getopt(argc, argv, "fp:s")) != -1) {
		switch(ch) {
		case 'f':
			force = 1;
			break;
		case 'p':
			parts = atoi(optarg);
			break;
		case 's':
			slice = 1;
			break;
		default:
			return usage();
		}
	}

	if (argc != optind)
		return usage();

	return migrate(gpt);
}

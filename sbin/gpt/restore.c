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
__FBSDID("$FreeBSD: src/sbin/gpt/create.c,v 1.11 2005/08/31 01:47:19 marcel Exp $");
#endif
#ifdef __RCSID
__RCSID("$NetBSD: restore.c,v 1.12 2015/12/02 04:07:11 christos Exp $");
#endif

#include <sys/types.h>
#include <sys/bootblock.h>
#include <sys/disklabel_gpt.h>

#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <prop/proplib.h>

#include "map.h"
#include "gpt.h"
#include "gpt_private.h"

static int force;

static int cmd_restore(gpt_t, int, char *[]);

static const char *restorehelp[] = {
    "[-F]",
};

struct gpt_cmd c_restore = {
	"restore",
	cmd_restore,
	restorehelp, __arraycount(restorehelp),
	0,
};

#define usage() gpt_usage(NULL, &c_restore)

#define PROP_ERR(x)     if (!(x)) {		\
	gpt_warnx(gpt, "proplib failure");	\
	return -1;				\
}

static int
restore(gpt_t gpt)
{
	gpt_uuid_t gpt_guid, uuid;
	off_t firstdata, last, lastdata, gpe_start, gpe_end;
	map_t map;
	struct mbr *mbr;
	struct gpt_hdr *hdr;
	struct gpt_ent ent;
	unsigned int i;
	prop_dictionary_t props, gpt_dict, mbr_dict, type_dict;
	prop_object_iterator_t propiter;
	prop_data_t propdata;
	prop_array_t mbr_array, gpt_array;
	prop_number_t propnum;
	prop_string_t propstr;
	int entries, gpt_size;
	const char *s;
	void *secbuf;

	last = gpt->mediasz / gpt->secsz - 1LL;

	if (map_find(gpt, MAP_TYPE_PRI_GPT_HDR) != NULL ||
	    map_find(gpt, MAP_TYPE_SEC_GPT_HDR) != NULL) {
		if (!force) {
			gpt_warnx(gpt, "Device contains a GPT");
			return -1;
		}
	}
	map = map_find(gpt, MAP_TYPE_MBR);
	if (map != NULL) {
		if (!force) {
			gpt_warnx(gpt, "Device contains an MBR");
			return -1;
		}
		/* Nuke the MBR in our internal map. */
		map->map_type = MAP_TYPE_UNUSED;
	}

	props = prop_dictionary_internalize_from_file("/dev/stdin");
	if (props == NULL) {
		gpt_warnx(gpt, "Unable to read/parse backup file");
		return -1;
	}

	propnum = prop_dictionary_get(props, "sector_size");
	PROP_ERR(propnum);
	if (!prop_number_equals_integer(propnum, gpt->secsz)) {
		gpt_warnx(gpt, "Sector size does not match backup");
		prop_object_release(props);
		return -1;
	}

	gpt_dict = prop_dictionary_get(props, "GPT_HDR");
	PROP_ERR(gpt_dict);

	propnum = prop_dictionary_get(gpt_dict, "revision");
	PROP_ERR(propnum);
	if (!prop_number_equals_unsigned_integer(propnum, 0x10000)) {
		gpt_warnx(gpt, "backup is not revision 1.0");
		prop_object_release(gpt_dict);
		prop_object_release(props);
		return -1;
	}

	propnum = prop_dictionary_get(gpt_dict, "entries");
	PROP_ERR(propnum);
	entries = prop_number_integer_value(propnum);
	gpt_size = entries * sizeof(struct gpt_ent) / gpt->secsz;
	if (gpt_size * sizeof(struct gpt_ent) % gpt->secsz)
		gpt_size++;

	propstr = prop_dictionary_get(gpt_dict, "guid");
	PROP_ERR(propstr);
	s = prop_string_cstring_nocopy(propstr);
	if (gpt_uuid_parse(s, gpt_guid) != 0) {
		gpt_warnx(gpt, "%s: not able to convert to an UUID", s);
		// XXX: leak
		return -1;
	}
	firstdata = gpt_size + 2;		/* PMBR and GPT header */
	lastdata = last - gpt_size - 1;		/* alt. GPT table and header */

	type_dict = prop_dictionary_get(props, "GPT_TBL");
	PROP_ERR(type_dict);
	gpt_array = prop_dictionary_get(type_dict, "gpt_array");
	PROP_ERR(gpt_array);
	propiter = prop_array_iterator(gpt_array);
	PROP_ERR(propiter);
	while ((gpt_dict = prop_object_iterator_next(propiter)) != NULL) {
		propstr = prop_dictionary_get(gpt_dict, "type");
		PROP_ERR(propstr);
		s = prop_string_cstring_nocopy(propstr);
		if (gpt_uuid_parse(s, uuid) != 0) {
			gpt_warnx(gpt, "%s: not able to convert to an UUID", s);
			return -1;
		}
		if (gpt_uuid_is_nil(uuid))
			continue;
		propnum = prop_dictionary_get(gpt_dict, "start");
		PROP_ERR(propnum);
		gpe_start = prop_number_unsigned_integer_value(propnum);
		propnum = prop_dictionary_get(gpt_dict, "end");
		PROP_ERR(propnum);
		gpe_end = prop_number_unsigned_integer_value(propnum);
		if (gpe_start < firstdata || gpe_end > lastdata) {
			gpt_warnx(gpt, "Backup GPT doesn't fit");
			return -1;
		}
	}
	prop_object_iterator_release(propiter);

	secbuf = calloc(gpt_size + 1, gpt->secsz);	/* GPT TABLE + GPT HEADER */
	if (secbuf == NULL) {
		gpt_warnx(gpt, "not enough memory to create a sector buffer");
		return -1;
	}

	if (lseek(gpt->fd, 0LL, SEEK_SET) == -1) {
		gpt_warnx(gpt, "Can't seek to beginning");
		return -1;
	}
	for (i = 0; i < firstdata; i++) {
		if (write(gpt->fd, secbuf, gpt->secsz) == -1) {
			gpt_warnx(gpt, "Error writing");
			return -1;
		}
	}
	if (lseek(gpt->fd, (lastdata + 1) * gpt->secsz, SEEK_SET) == -1) {
		gpt_warnx(gpt, "Can't seek to end");
		return -1;
	}
	for (i = lastdata + 1; i <= last; i++) {
		if (write(gpt->fd, secbuf, gpt->secsz) == -1) {
			gpt_warnx(gpt, "Error writing");
			return -1;
		}
	}

	mbr = (struct mbr *)secbuf;
	type_dict = prop_dictionary_get(props, "MBR");
	PROP_ERR(type_dict);
	propdata = prop_dictionary_get(type_dict, "code");
	PROP_ERR(propdata);
	memcpy(mbr->mbr_code, prop_data_data_nocopy(propdata),
	    sizeof(mbr->mbr_code));
	mbr_array = prop_dictionary_get(type_dict, "mbr_array");
	PROP_ERR(mbr_array);
	propiter = prop_array_iterator(mbr_array);
	PROP_ERR(propiter);
	while ((mbr_dict = prop_object_iterator_next(propiter)) != NULL) {
		propnum = prop_dictionary_get(mbr_dict, "index");
		PROP_ERR(propnum);
		i = prop_number_integer_value(propnum);
		propnum = prop_dictionary_get(mbr_dict, "flag");
		PROP_ERR(propnum);
		mbr->mbr_part[i].part_flag =
		    prop_number_unsigned_integer_value(propnum);
		propnum = prop_dictionary_get(mbr_dict, "start_head");
		PROP_ERR(propnum);
		mbr->mbr_part[i].part_shd =
		    prop_number_unsigned_integer_value(propnum);
		propnum = prop_dictionary_get(mbr_dict, "start_sector");
		PROP_ERR(propnum);
		mbr->mbr_part[i].part_ssect =
		    prop_number_unsigned_integer_value(propnum);
		propnum = prop_dictionary_get(mbr_dict, "start_cylinder");
		PROP_ERR(propnum);
		mbr->mbr_part[i].part_scyl =
		    prop_number_unsigned_integer_value(propnum);
		propnum = prop_dictionary_get(mbr_dict, "type");
		PROP_ERR(propnum);
		mbr->mbr_part[i].part_typ =
		    prop_number_unsigned_integer_value(propnum);
		propnum = prop_dictionary_get(mbr_dict, "end_head");
		PROP_ERR(propnum);
		mbr->mbr_part[i].part_ehd =
		    prop_number_unsigned_integer_value(propnum);
		propnum = prop_dictionary_get(mbr_dict, "end_sector");
		PROP_ERR(propnum);
		mbr->mbr_part[i].part_esect =
		    prop_number_unsigned_integer_value(propnum);
		propnum = prop_dictionary_get(mbr_dict, "end_cylinder");
		PROP_ERR(propnum);
		mbr->mbr_part[i].part_ecyl =
		    prop_number_unsigned_integer_value(propnum);
		propnum = prop_dictionary_get(mbr_dict, "lba_start_low");
		PROP_ERR(propnum);
		mbr->mbr_part[i].part_start_lo =
		    htole16(prop_number_unsigned_integer_value(propnum));
		propnum = prop_dictionary_get(mbr_dict, "lba_start_high");
		PROP_ERR(propnum);
		mbr->mbr_part[i].part_start_hi =
		    htole16(prop_number_unsigned_integer_value(propnum));
		/* adjust PMBR size to size of device */
                if (mbr->mbr_part[i].part_typ == MBR_PTYPE_PMBR) {
			if (last > 0xffffffff) {
				mbr->mbr_part[0].part_size_lo = htole16(0xffff);
				mbr->mbr_part[0].part_size_hi = htole16(0xffff);
			} else {
				mbr->mbr_part[0].part_size_lo = htole16(last);
				mbr->mbr_part[0].part_size_hi =
				    htole16(last >> 16);
			}
		} else {
			propnum = prop_dictionary_get(mbr_dict, "lba_size_low");
			PROP_ERR(propnum);
			mbr->mbr_part[i].part_size_lo =
			    htole16(prop_number_unsigned_integer_value(propnum));
			propnum =
			    prop_dictionary_get(mbr_dict, "lba_size_high");
			PROP_ERR(propnum);
			mbr->mbr_part[i].part_size_hi =
			    htole16(prop_number_unsigned_integer_value(propnum));
		}
	}
	prop_object_iterator_release(propiter);
	mbr->mbr_sig = htole16(MBR_SIG);
	if (lseek(gpt->fd, 0LL, SEEK_SET) == -1 ||
	    write(gpt->fd, mbr, gpt->secsz) == -1) {
		gpt_warnx(gpt, "Unable to write MBR");
		return -1;
	}
	
	propiter = prop_array_iterator(gpt_array);
	PROP_ERR(propiter);
	while ((gpt_dict = prop_object_iterator_next(propiter)) != NULL) {
		memset(&ent, 0, sizeof(ent));
		propstr = prop_dictionary_get(gpt_dict, "type");
		PROP_ERR(propstr);
		s = prop_string_cstring_nocopy(propstr);
		if (gpt_uuid_parse(s, ent.ent_type) != 0) {
			gpt_warnx(gpt, "%s: not able to convert to an UUID", s);
			return -1;
		}
		propstr = prop_dictionary_get(gpt_dict, "guid");
		PROP_ERR(propstr);
		s = prop_string_cstring_nocopy(propstr);
		if (gpt_uuid_parse(s, ent.ent_guid) != 0) {
			gpt_warnx(gpt, "%s: not able to convert to an UUID", s);
			return -1;
		}
		propnum = prop_dictionary_get(gpt_dict, "start");
		PROP_ERR(propnum);
		ent.ent_lba_start =
		    htole64(prop_number_unsigned_integer_value(propnum));
		propnum = prop_dictionary_get(gpt_dict, "end");
		PROP_ERR(propnum);
		ent.ent_lba_end =
		    htole64(prop_number_unsigned_integer_value(propnum));
		propnum = prop_dictionary_get(gpt_dict, "attributes");
		PROP_ERR(propnum);
		ent.ent_attr =
		    htole64(prop_number_unsigned_integer_value(propnum));
		propstr = prop_dictionary_get(gpt_dict, "name");
		if (propstr != NULL) {
			s = prop_string_cstring_nocopy(propstr);
			utf8_to_utf16((const uint8_t *)s, ent.ent_name,
				__arraycount(ent.ent_name));
		}
		propnum = prop_dictionary_get(gpt_dict, "index");
		PROP_ERR(propnum);
		i = prop_number_integer_value(propnum);
		memcpy((char *)secbuf + gpt->secsz + ((i - 1) * sizeof(ent)),
		    &ent, sizeof(ent));
	}
	prop_object_iterator_release(propiter);
	if (lseek(gpt->fd, 2 * gpt->secsz, SEEK_SET) == -1 ||
	    write(gpt->fd, (char *)secbuf + 1 * gpt->secsz,
	    gpt_size * gpt->secsz) == -1) {
		gpt_warnx(gpt, "Unable to write primary GPT");
		return -1;
	}
	if (lseek(gpt->fd, (lastdata + 1) * gpt->secsz, SEEK_SET) == -1 ||
	    write(gpt->fd, (char *)secbuf + 1 * gpt->secsz,
	    gpt_size * gpt->secsz) == -1) {
		gpt_warnx(gpt, "Unable to write secondary GPT");
		return -1;
	}

	memset(secbuf, 0, gpt->secsz);
	hdr = (struct gpt_hdr *)secbuf;
	memcpy(hdr->hdr_sig, GPT_HDR_SIG, sizeof(hdr->hdr_sig));
	hdr->hdr_revision = htole32(GPT_HDR_REVISION);
	hdr->hdr_size = htole32(GPT_HDR_SIZE);
	hdr->hdr_lba_self = htole64(GPT_HDR_BLKNO);
	hdr->hdr_lba_alt = htole64(last);
	hdr->hdr_lba_start = htole64(firstdata);
	hdr->hdr_lba_end = htole64(lastdata);
	gpt_uuid_copy(hdr->hdr_guid, gpt_guid);
	hdr->hdr_lba_table = htole64(2);
	hdr->hdr_entries = htole32(entries);
	hdr->hdr_entsz = htole32(sizeof(struct gpt_ent));
	hdr->hdr_crc_table = htole32(crc32((char *)secbuf + 1 * gpt->secsz,
	    gpt_size * gpt->secsz));
	hdr->hdr_crc_self = htole32(crc32(hdr, GPT_HDR_SIZE));
	if (lseek(gpt->fd, 1 * gpt->secsz, SEEK_SET) == -1 ||
	    write(gpt->fd, hdr, gpt->secsz) == -1) {
		gpt_warnx(gpt, "Unable to write primary header");
		return -1;
	}

	hdr->hdr_lba_self = htole64(last);
	hdr->hdr_lba_alt = htole64(GPT_HDR_BLKNO);
	hdr->hdr_lba_table = htole64(lastdata + 1);
	hdr->hdr_crc_self = 0;
	hdr->hdr_crc_self = htole32(crc32(hdr, GPT_HDR_SIZE));
	if (lseek(gpt->fd, last * gpt->secsz, SEEK_SET) == -1 ||
	    write(gpt->fd, hdr, gpt->secsz) == -1) {
		gpt_warnx(gpt, "Unable to write secondary header");
		return -1;
	}

	prop_object_release(props);
	return 0;
}

static int
cmd_restore(gpt_t gpt, int argc, char *argv[])
{
	int ch;

	while ((ch = getopt(argc, argv, "F")) != -1) {
		switch(ch) {
		case 'F':
			force = 1;
			break;
		default:
			return usage();
		}
	}

	if (argc != optind)
		return usage();

	return restore(gpt);
}

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
__FBSDID("$FreeBSD: src/sbin/gpt/show.c,v 1.14 2006/06/22 22:22:32 marcel Exp $");
#endif
#ifdef __RCSID
__RCSID("$NetBSD: backup.c,v 1.18 2017/09/07 10:23:33 christos Exp $");
#endif

#include <sys/bootblock.h>
#include <sys/types.h>

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

static const char *backuphelp[] = {
	"[-o outfile]",
};

static int cmd_backup(gpt_t, int, char *[]);

struct gpt_cmd c_backup = {
	"backup",
	cmd_backup,
	backuphelp, __arraycount(backuphelp),
	GPT_READONLY,
};

#define usage() gpt_usage(NULL, &c_backup)

#define PROP_ERR(x)	if (!(x)) goto cleanup

#define prop_uint(a) prop_number_create_unsigned_integer(a)

static int
store_mbr(gpt_t gpt, unsigned int i, const struct mbr *mbr,
    prop_array_t *mbr_array)
{
	prop_dictionary_t mbr_dict;
	prop_number_t propnum;
	const struct mbr_part *par = &mbr->mbr_part[i];
	bool rc;

	if (mbr->mbr_part[i].part_typ == MBR_PTYPE_UNUSED)
		return 0;

	mbr_dict = prop_dictionary_create();
	PROP_ERR(mbr_dict);
	propnum = prop_number_create_integer(i);
	PROP_ERR(propnum);
	rc = prop_dictionary_set(mbr_dict, "index", propnum);
	PROP_ERR(rc);
	propnum = prop_uint(par->part_flag);
	PROP_ERR(propnum);
	rc = prop_dictionary_set(mbr_dict, "flag", propnum);
	PROP_ERR(rc);
	propnum = prop_uint(par->part_shd);
	PROP_ERR(propnum);
	rc = prop_dictionary_set(mbr_dict, "start_head", propnum);
	PROP_ERR(rc);
	propnum = prop_uint(par->part_ssect);
	PROP_ERR(propnum);
	rc = prop_dictionary_set(mbr_dict, "start_sector", propnum);
	PROP_ERR(rc);
	propnum = prop_uint(par->part_scyl);
	PROP_ERR(propnum);
	rc = prop_dictionary_set(mbr_dict, "start_cylinder", propnum);
	PROP_ERR(rc);
	propnum = prop_uint(par->part_typ);
	PROP_ERR(propnum);
	rc = prop_dictionary_set(mbr_dict, "type", propnum);
	PROP_ERR(rc);
	propnum = prop_uint(par->part_ehd);
	PROP_ERR(propnum);
	rc = prop_dictionary_set(mbr_dict, "end_head", propnum);
	PROP_ERR(rc);
	propnum = prop_uint(par->part_esect);
	PROP_ERR(propnum);
	rc = prop_dictionary_set(mbr_dict, "end_sector", propnum);
	PROP_ERR(rc);
	propnum = prop_uint(par->part_ecyl);
	PROP_ERR(propnum);
	rc = prop_dictionary_set(mbr_dict, "end_cylinder", propnum);
	PROP_ERR(rc);
	propnum = prop_uint(le16toh(par->part_start_lo));
	PROP_ERR(propnum);
	rc = prop_dictionary_set(mbr_dict, "lba_start_low", propnum);
	PROP_ERR(rc);
	propnum = prop_uint(le16toh(par->part_start_hi));
	PROP_ERR(propnum);
	rc = prop_dictionary_set(mbr_dict, "lba_start_high", propnum);
	PROP_ERR(rc);
	propnum = prop_uint(le16toh(par->part_size_lo));
	PROP_ERR(propnum);
	rc = prop_dictionary_set(mbr_dict, "lba_size_low", propnum);
	PROP_ERR(rc);
	propnum = prop_uint(le16toh(par->part_size_hi));
	PROP_ERR(propnum);
	rc = prop_dictionary_set(mbr_dict, "lba_size_high", propnum);
	if (*mbr_array == NULL) {
		*mbr_array = prop_array_create();
		PROP_ERR(*mbr_array);
	}
	rc = prop_array_add(*mbr_array, mbr_dict);
	PROP_ERR(rc);
	return 0;
cleanup:
	if (mbr_dict)
		prop_object_release(mbr_dict);
	gpt_warnx(gpt, "proplib failure");
	return -1;
}

static int
store_gpt(gpt_t gpt, const struct gpt_hdr *hdr, prop_dictionary_t *type_dict)
{
	prop_number_t propnum;
	prop_string_t propstr;
	char buf[128];
	bool rc;

	*type_dict = prop_dictionary_create();
	PROP_ERR(type_dict);
	propnum = prop_uint(le32toh(hdr->hdr_revision));
	PROP_ERR(propnum);
	rc = prop_dictionary_set(*type_dict, "revision", propnum);
	PROP_ERR(rc);
	gpt_uuid_snprintf(buf, sizeof(buf), "%d", hdr->hdr_guid);
	propstr = prop_string_create_cstring(buf);
	PROP_ERR(propstr);
	rc = prop_dictionary_set(*type_dict, "guid", propstr);
	PROP_ERR(rc);
	propnum = prop_number_create_integer(le32toh(hdr->hdr_entries));
	PROP_ERR(propnum);
	rc = prop_dictionary_set(*type_dict, "entries", propnum);
	PROP_ERR(rc);
	return 0;
cleanup:
	if (*type_dict)
		prop_object_release(*type_dict);
	return -1;
}

static int
store_tbl(gpt_t gpt, const map_t m, prop_dictionary_t *type_dict)
{
	const struct gpt_ent *ent;
	unsigned int i;
	prop_dictionary_t gpt_dict;
	prop_array_t gpt_array;
	prop_number_t propnum;
	prop_string_t propstr;
	char buf[128];
	uint8_t utfbuf[__arraycount(ent->ent_name) * 3 + 1];
	bool rc;

	*type_dict = NULL;

	gpt_array = prop_array_create();
	PROP_ERR(gpt_array);

	*type_dict = prop_dictionary_create();
	PROP_ERR(*type_dict);

	ent = m->map_data;
	for (i = 1, ent = m->map_data;
	    (const char *)ent < (const char *)(m->map_data) +
	    m->map_size * gpt->secsz; i++, ent++) {
		gpt_dict = prop_dictionary_create();
		PROP_ERR(gpt_dict);
		propnum = prop_number_create_integer(i);
		PROP_ERR(propnum);
		rc = prop_dictionary_set(gpt_dict, "index", propnum);
		PROP_ERR(propnum);
		gpt_uuid_snprintf(buf, sizeof(buf), "%d", ent->ent_type);
		propstr = prop_string_create_cstring(buf);
		PROP_ERR(propstr);
		rc = prop_dictionary_set(gpt_dict, "type", propstr);
		gpt_uuid_snprintf(buf, sizeof(buf), "%d", ent->ent_guid);
		propstr = prop_string_create_cstring(buf);
		PROP_ERR(propstr);
		rc = prop_dictionary_set(gpt_dict, "guid", propstr);
		PROP_ERR(propstr);
		propnum = prop_uint(le64toh(ent->ent_lba_start));
		PROP_ERR(propnum);
		rc = prop_dictionary_set(gpt_dict, "start", propnum);
		PROP_ERR(rc);
		propnum = prop_uint(le64toh(ent->ent_lba_end));
		PROP_ERR(rc);
		rc = prop_dictionary_set(gpt_dict, "end", propnum);
		PROP_ERR(rc);
		propnum = prop_uint(le64toh(ent->ent_attr));
		PROP_ERR(propnum);
		rc = prop_dictionary_set(gpt_dict, "attributes", propnum);
		PROP_ERR(rc);
		utf16_to_utf8(ent->ent_name, __arraycount(ent->ent_name),
		    utfbuf, __arraycount(utfbuf));
		if (utfbuf[0] != '\0') {
			propstr = prop_string_create_cstring((char *)utfbuf);
			PROP_ERR(propstr);
			rc = prop_dictionary_set(gpt_dict, "name", propstr);
			PROP_ERR(rc);
		}
		rc = prop_array_add(gpt_array, gpt_dict);
		PROP_ERR(rc);
	}
	rc = prop_dictionary_set(*type_dict, "gpt_array", gpt_array);
	PROP_ERR(rc);
	prop_object_release(gpt_array);
	return 0;
cleanup:
	if (*type_dict)
		prop_object_release(*type_dict);
	if (gpt_array)
		prop_object_release(gpt_array);
	return -1;
}

static int
backup(gpt_t gpt, const char *outfile)
{
	map_t m;
	struct mbr *mbr;
	unsigned int i;
	prop_dictionary_t props, type_dict;
	prop_array_t mbr_array;
	prop_data_t propdata;
	prop_number_t propnum;
	char *propext;
	bool rc;
	FILE *fp;

	props = prop_dictionary_create();
	PROP_ERR(props);
	propnum = prop_number_create_integer(gpt->secsz);
	PROP_ERR(propnum);
	rc = prop_dictionary_set(props, "sector_size", propnum);
	PROP_ERR(rc);
	m = map_first(gpt);
	while (m != NULL) {
		switch (m->map_type) {
		case MAP_TYPE_MBR:
		case MAP_TYPE_PMBR:
			type_dict = prop_dictionary_create();
			PROP_ERR(type_dict);
			mbr = m->map_data;
			propdata = prop_data_create_data_nocopy(mbr->mbr_code,
			    sizeof(mbr->mbr_code));
			PROP_ERR(propdata);
			rc = prop_dictionary_set(type_dict, "code", propdata);
			PROP_ERR(rc);
			mbr_array = NULL;
			for (i = 0; i < 4; i++) {
				if (store_mbr(gpt, i, mbr, &mbr_array) == -1)
					goto cleanup;
			}
			if (mbr_array != NULL) {
				rc = prop_dictionary_set(type_dict,
				    "mbr_array", mbr_array);
				PROP_ERR(rc);
				prop_object_release(mbr_array);
			}
			rc = prop_dictionary_set(props, "MBR", type_dict);
			PROP_ERR(rc);
			prop_object_release(type_dict);
			break;
		case MAP_TYPE_PRI_GPT_HDR:
			if (store_gpt(gpt, m->map_data, &type_dict) == -1)
				goto cleanup;

			rc = prop_dictionary_set(props, "GPT_HDR", type_dict);
			PROP_ERR(rc);
			prop_object_release(type_dict);
			break;
		case MAP_TYPE_PRI_GPT_TBL:
			if (store_tbl(gpt, m, &type_dict) == -1)
				goto cleanup;
			rc = prop_dictionary_set(props, "GPT_TBL", type_dict);
			PROP_ERR(rc);
			prop_object_release(type_dict);
			break;
		}
		m = m->map_next;
	}
	propext = prop_dictionary_externalize(props);
	PROP_ERR(propext);
	prop_object_release(props);
	fp = strcmp(outfile, "-") == 0 ? stdout : fopen(outfile, "w");
	if (fp == NULL) {
		gpt_warn(gpt, "Can't open `%s'", outfile);
		free(propext);
		goto cleanup;
	}
	fputs(propext, fp);
	if (fp != stdout)
		fclose(fp);
	free(propext);
	return 0;
cleanup:
	if (props)
		prop_object_release(props);
	return -1;
}

static int
cmd_backup(gpt_t gpt, int argc, char *argv[])
{
	int ch;
	const char *outfile = "-";

	while ((ch = getopt(argc, argv, "o:")) != -1) {
		switch(ch) {
		case 'o':
			outfile = optarg;
			break;
		default:
			return usage();
		}
	}
	if (argc != optind)
		return usage();

	return backup(gpt, outfile);
}

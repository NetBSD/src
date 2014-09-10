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

#include <sys/cdefs.h>
#ifdef __FBSDID
__FBSDID("$FreeBSD: src/sbin/gpt/show.c,v 1.14 2006/06/22 22:22:32 marcel Exp $");
#endif
#ifdef __RCSID
__RCSID("$NetBSD: backup.c,v 1.3 2014/09/10 10:49:44 jnemeth Exp $");
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


const char backupmsg[] = "backup device ...";

__dead static void
usage_backup(void)
{

	fprintf(stderr,
	    "usage: %s %s\n", getprogname(), backupmsg);
	exit(1);
}

#define PROP_ERR(x)	if (!(x)) {		\
		warn("proplib failure");	\
		return;				\
	}

static void
backup(void)
{
	uuid_t u;
	map_t *m;
	struct mbr *mbr;
	struct gpt_ent *ent;
	struct gpt_hdr *hdr;
	unsigned int i;
	prop_dictionary_t props, mbr_dict, gpt_dict, type_dict;
	prop_array_t mbr_array, gpt_array;
	prop_data_t propdata;
	prop_number_t propnum;
	prop_string_t propstr;
	char *propext, *s;
	bool rc;

	props = prop_dictionary_create();
	PROP_ERR(props);
	propnum = prop_number_create_integer(secsz);
	PROP_ERR(propnum);
	rc = prop_dictionary_set(props, "sector_size", propnum);
	PROP_ERR(rc);
	m = map_first();
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
				if (mbr->mbr_part[i].part_typ !=
				    MBR_PTYPE_UNUSED) {
					mbr_dict = prop_dictionary_create();
					PROP_ERR(mbr_dict);
					propnum = prop_number_create_integer(i);
					PROP_ERR(propnum);
					rc = prop_dictionary_set(mbr_dict,
					    "index", propnum);
					PROP_ERR(rc);
					propnum = prop_number_create_unsigned_integer(mbr->mbr_part[i].part_flag);
					PROP_ERR(propnum);
					rc = prop_dictionary_set(mbr_dict,
					    "flag", propnum);
					PROP_ERR(rc);
					propnum = prop_number_create_unsigned_integer(mbr->mbr_part[i].part_shd);
					PROP_ERR(propnum);
					rc = prop_dictionary_set(mbr_dict,
					    "start_head", propnum);
					PROP_ERR(rc);
					propnum = prop_number_create_unsigned_integer(mbr->mbr_part[i].part_ssect);
					PROP_ERR(propnum);
					rc = prop_dictionary_set(mbr_dict,
					    "start_sector", propnum);
					PROP_ERR(rc);
					propnum = prop_number_create_unsigned_integer(mbr->mbr_part[i].part_scyl);
					PROP_ERR(propnum);
					rc = prop_dictionary_set(mbr_dict,
					    "start_cylinder", propnum);
					PROP_ERR(rc);
					propnum = prop_number_create_unsigned_integer(mbr->mbr_part[i].part_typ);
					PROP_ERR(propnum);
					rc = prop_dictionary_set(mbr_dict,
					    "type", propnum);
					PROP_ERR(rc);
					propnum = prop_number_create_unsigned_integer(mbr->mbr_part[i].part_ehd);
					PROP_ERR(propnum);
					rc = prop_dictionary_set(mbr_dict,
					    "end_head", propnum);
					PROP_ERR(rc);
					propnum = prop_number_create_unsigned_integer(mbr->mbr_part[i].part_esect);
					PROP_ERR(propnum);
					rc = prop_dictionary_set(mbr_dict,
					    "end_sector", propnum);
					PROP_ERR(rc);
					propnum = prop_number_create_unsigned_integer(mbr->mbr_part[i].part_ecyl);
					PROP_ERR(propnum);
					rc = prop_dictionary_set(mbr_dict,
					    "end_cylinder", propnum);
					PROP_ERR(rc);
					propnum = prop_number_create_unsigned_integer(le16toh(mbr->mbr_part[i].part_start_lo));
					PROP_ERR(propnum);
					rc = prop_dictionary_set(mbr_dict,
					    "lba_start_low", propnum);
					PROP_ERR(rc);
					propnum = prop_number_create_unsigned_integer(le16toh(mbr->mbr_part[i].part_start_hi));
					PROP_ERR(propnum);
					rc = prop_dictionary_set(mbr_dict,
					    "lba_start_high", propnum);
					PROP_ERR(rc);
					propnum = prop_number_create_unsigned_integer(le16toh(mbr->mbr_part[i].part_size_lo));
					PROP_ERR(propnum);
					rc = prop_dictionary_set(mbr_dict,
					    "lba_size_low", propnum);
					PROP_ERR(rc);
					propnum = prop_number_create_unsigned_integer(le16toh(mbr->mbr_part[i].part_size_hi));
					PROP_ERR(propnum);
					rc = prop_dictionary_set(mbr_dict,
					    "lba_size_high", propnum);
					if (mbr_array == NULL) {
						mbr_array = prop_array_create();
						PROP_ERR(mbr_array);
					}
					rc = prop_array_add(mbr_array,
					    mbr_dict);
					PROP_ERR(rc);
				}
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
			type_dict = prop_dictionary_create();
			PROP_ERR(type_dict);
			hdr = m->map_data;
			propnum = prop_number_create_unsigned_integer(le32toh(hdr->hdr_revision));
			PROP_ERR(propnum);
			rc = prop_dictionary_set(type_dict, "revision",
			    propnum);
			PROP_ERR(rc);
			le_uuid_dec(hdr->hdr_guid, &u);
			uuid_to_string(&u, &s, NULL);
			propstr = prop_string_create_cstring(s);
			free(s);
			PROP_ERR(propstr);
			rc = prop_dictionary_set(type_dict, "guid", propstr);
			PROP_ERR(rc);
			propnum = prop_number_create_integer(le32toh(hdr->hdr_entries));
			PROP_ERR(propnum);
			rc = prop_dictionary_set(type_dict, "entries", propnum);
			PROP_ERR(rc);
			rc = prop_dictionary_set(props, "GPT_HDR", type_dict);
			PROP_ERR(rc);
			prop_object_release(type_dict);
			break;
		case MAP_TYPE_PRI_GPT_TBL:
			type_dict = prop_dictionary_create();
			PROP_ERR(type_dict);
			ent = m->map_data;
			gpt_array = prop_array_create();
			PROP_ERR(gpt_array);
			for (i = 1, ent = m->map_data;
			    (char *)ent < (char *)(m->map_data) +
			    m->map_size * secsz; i++, ent++) {
				gpt_dict = prop_dictionary_create();
				PROP_ERR(gpt_dict);
				propnum = prop_number_create_integer(i);
				PROP_ERR(propnum);
				rc = prop_dictionary_set(gpt_dict, "index",
				    propnum);
				PROP_ERR(propnum);
				le_uuid_dec(ent->ent_type, &u);
				uuid_to_string(&u, &s, NULL);
				propstr = prop_string_create_cstring(s);
				free(s);
				PROP_ERR(propstr);
				rc = prop_dictionary_set(gpt_dict, "type",
				    propstr);
				le_uuid_dec(ent->ent_guid, &u);
				uuid_to_string(&u, &s, NULL);
				propstr = prop_string_create_cstring(s);
				free(s);
				PROP_ERR(propstr);
				rc = prop_dictionary_set(gpt_dict, "guid",
				    propstr);
				PROP_ERR(propstr);
				propnum = prop_number_create_unsigned_integer(le64toh(ent->ent_lba_start));
				PROP_ERR(propnum);
				rc = prop_dictionary_set(gpt_dict, "start",
				    propnum);
				PROP_ERR(rc);
				propnum = prop_number_create_unsigned_integer(le64toh(ent->ent_lba_end));
				PROP_ERR(rc);
				rc = prop_dictionary_set(gpt_dict, "end",
				    propnum);
				PROP_ERR(rc);
				propnum = prop_number_create_unsigned_integer(le64toh(ent->ent_attr));
				PROP_ERR(propnum);
				rc = prop_dictionary_set(gpt_dict,
				    "attributes", propnum);
				PROP_ERR(rc);
				s = (char *)utf16_to_utf8(ent->ent_name);
				if (*s != '\0') {
					propstr = prop_string_create_cstring(s);
					PROP_ERR(propstr);
					rc = prop_dictionary_set(gpt_dict,
					    "name", propstr);
					PROP_ERR(rc);
				}
				rc = prop_array_add(gpt_array, gpt_dict);
				PROP_ERR(rc);
			}
			rc = prop_dictionary_set(type_dict,
			    "gpt_array", gpt_array);
			PROP_ERR(rc);
			prop_object_release(gpt_array);
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
	fputs(propext, stdout);
	free(propext);
}

int
cmd_backup(int argc, char *argv[])
{
	int fd;

	if (argc == optind)
		usage_backup();

	while (optind < argc) {
		fd = gpt_open(argv[optind++]);
		if (fd == -1) {
			warn("unable to open device '%s'", device_name);
			continue;
		}

		backup();

		gpt_close(fd);
	}

	return (0);
}

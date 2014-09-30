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
__RCSID("$NetBSD: show.c,v 1.21 2014/09/30 22:56:36 jnemeth Exp $");
#endif

#include <sys/types.h>

#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "map.h"
#include "gpt.h"

static int show_label = 0;
static int show_uuid = 0;
static int show_guid = 0;
static unsigned int entry = 0;

const char showmsg[] = "show [-glu] [-i index] device ...";

__dead static void
usage_show(void)
{

	fprintf(stderr,
	    "usage: %s %s\n", getprogname(), showmsg);
	exit(1);
}

static void
show(void)
{
	off_t start;
	map_t *m, *p;
	struct mbr *mbr;
	struct gpt_ent *ent;
	unsigned int i;

	printf("  %*s", lbawidth, "start");
	printf("  %*s", lbawidth, "size");
	printf("  index  contents\n");

	m = map_first();
	while (m != NULL) {
		printf("  %*llu", lbawidth, (long long)m->map_start);
		printf("  %*llu", lbawidth, (long long)m->map_size);
		putchar(' ');
		putchar(' ');
		if (m->map_index > 0)
			printf("%5d", m->map_index);
		else
			printf("     ");
		putchar(' ');
		putchar(' ');
		switch (m->map_type) {
		case MAP_TYPE_MBR:
			if (m->map_start != 0)
				printf("Extended ");
			printf("MBR");
			break;
		case MAP_TYPE_PRI_GPT_HDR:
			printf("Pri GPT header");
			break;
		case MAP_TYPE_SEC_GPT_HDR:
			printf("Sec GPT header");
			break;
		case MAP_TYPE_PRI_GPT_TBL:
			printf("Pri GPT table");
			break;
		case MAP_TYPE_SEC_GPT_TBL:
			printf("Sec GPT table");
			break;
		case MAP_TYPE_MBR_PART:
			p = m->map_data;
			if (p->map_start != 0)
				printf("Extended ");
			printf("MBR part ");
			mbr = p->map_data;
			for (i = 0; i < 4; i++) {
				start = le16toh(mbr->mbr_part[i].part_start_hi);
				start = (start << 16) +
				    le16toh(mbr->mbr_part[i].part_start_lo);
				if (m->map_start == p->map_start + start)
					break;
			}
			printf("%d", mbr->mbr_part[i].part_typ);
			break;
		case MAP_TYPE_GPT_PART:
			printf("GPT part ");
			ent = m->map_data;
			if (show_label) {
				printf("- \"%s\"",
				    utf16_to_utf8(ent->ent_name));
			} else if (show_guid) {
				char buf[128];
				gpt_uuid_snprintf(
				    buf, sizeof(buf), "%d", ent->ent_guid);
				printf("- %s", buf);
			} else {
				char buf[128];
				if (show_uuid || gpt_uuid_snprintf(buf,
				    sizeof(buf), "%ls", ent->ent_type) == -1)
					gpt_uuid_snprintf(buf, sizeof(buf),
					    "%d", ent->ent_type);
				printf("- %s", buf);
			}
			break;
		case MAP_TYPE_PMBR:
			printf("PMBR");
			break;
		}
		putchar('\n');
		m = m->map_next;
	}
}

static void
show_one(void)
{
	map_t *m;
	struct gpt_ent *ent;
	char s1[128], s2[128];
#ifdef HN_AUTOSCALE
	char human_num[5];
#endif

	for (m = map_first(); m != NULL; m = m->map_next)
		if (entry == m->map_index)
			break;
	if (m == NULL) {
		warnx("%s: error: could not find index %d",
		    device_name, entry);
		return;
	}
	ent = m->map_data;

	printf("Details for index %d:\n", entry);
#ifdef HN_AUTOSCALE
	if (humanize_number(human_num, 5, (int64_t)(m->map_start * secsz),
	    "", HN_AUTOSCALE, HN_NOSPACE|HN_B) < 0)
		human_num[0] = '\0';
	if (human_num[0] != '\0')
		printf("Start: %llu (%s)\n", (long long)m->map_start,
		    human_num);
	else
#endif
		printf("Start: %llu\n", (long long)m->map_start);
#ifdef HN_AUTOSCALE
	if (humanize_number(human_num, 5, (int64_t)(m->map_size * secsz),
	    "", HN_AUTOSCALE, HN_NOSPACE|HN_B) < 0)
		human_num[0] = '\0';
	if (human_num[0] != '\0')
		printf("Size: %llu (%s)\n", (long long)m->map_size, human_num);
	else
#endif
		printf("Size: %llu\n", (long long)m->map_size);

	gpt_uuid_snprintf(s1, sizeof(s1), "%s", ent->ent_type);
	gpt_uuid_snprintf(s2, sizeof(s2), "%d", ent->ent_type);
	if (strcmp(s1, s2) == 0)
		strlcpy(s1, "unknown", sizeof(s1));
	printf("Type: %s (%s)\n", s1, s2);

	gpt_uuid_snprintf(s2, sizeof(s1), "%d", ent->ent_guid);
	printf("GUID: %s\n", s2);

	printf("Label: %s\n", utf16_to_utf8(ent->ent_name));

	printf("Attributes:\n");
	if (ent->ent_attr == 0)
		printf("  None\n");
	else {
		if (ent->ent_attr & GPT_ENT_ATTR_REQUIRED_PARTITION)
			printf("  required for platform to function\n");
		if (ent->ent_attr & GPT_ENT_ATTR_NO_BLOCK_IO_PROTOCOL)
			printf("  UEFI won't recognize file system\n");
		if (ent->ent_attr & GPT_ENT_ATTR_LEGACY_BIOS_BOOTABLE)
			printf("  legacy BIOS boot partition\n");
		if (ent->ent_attr & GPT_ENT_ATTR_BOOTME)
			printf("  indicates a bootable partition\n");
		if (ent->ent_attr & GPT_ENT_ATTR_BOOTONCE)
			printf("  attempt to boot this partition only once\n");
		if (ent->ent_attr & GPT_ENT_ATTR_BOOTFAILED)
			printf("  partition that was marked bootonce but failed to boot\n");
	}
}

int
cmd_show(int argc, char *argv[])
{
	char *p;
	int ch, fd;

	while ((ch = getopt(argc, argv, "gi:lu")) != -1) {
		switch(ch) {
		case 'g':
			show_guid = 1;
			break;
		case 'i':
			if (entry > 0)
				usage_show();
			entry = strtoul(optarg, &p, 10);
			if (*p != 0 || entry < 1)
				usage_show();
			break;
		case 'l':
			show_label = 1;
			break;
		case 'u':
			show_uuid = 1;
			break;
		default:
			usage_show();
		}
	}

	if (argc == optind)
		usage_show();

	while (optind < argc) {
		fd = gpt_open(argv[optind++]);
		if (fd == -1) {
			warn("unable to open device '%s'", device_name);
			continue;
		}

		if (entry > 0)
			show_one();
		else
			show();

		gpt_close(fd);
	}

	return (0);
}

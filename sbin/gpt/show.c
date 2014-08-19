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
__RCSID("$NetBSD: show.c,v 1.7.8.3 2014/08/20 00:02:25 tls Exp $");
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

static const char *
friendly(uuid_t *t)
{
	static const uuid_t efi_slice = GPT_ENT_TYPE_EFI;
	static const uuid_t bios_boot = GPT_ENT_TYPE_BIOS;
	static const uuid_t msdata = GPT_ENT_TYPE_MS_BASIC_DATA;
	static const uuid_t freebsd = GPT_ENT_TYPE_FREEBSD;
	static const uuid_t hfs = GPT_ENT_TYPE_APPLE_HFS;
	static const uuid_t linuxdata = GPT_ENT_TYPE_LINUX_DATA;
	static const uuid_t linuxswap = GPT_ENT_TYPE_LINUX_SWAP;
	static const uuid_t msr = GPT_ENT_TYPE_MS_RESERVED;
	static const uuid_t swap = GPT_ENT_TYPE_FREEBSD_SWAP;
	static const uuid_t ufs = GPT_ENT_TYPE_FREEBSD_UFS;
	static const uuid_t vinum = GPT_ENT_TYPE_FREEBSD_VINUM;
	static const uuid_t zfs = GPT_ENT_TYPE_FREEBSD_ZFS;
	static const uuid_t nb_swap = GPT_ENT_TYPE_NETBSD_SWAP;
	static const uuid_t nb_ffs = GPT_ENT_TYPE_NETBSD_FFS;
	static const uuid_t nb_lfs = GPT_ENT_TYPE_NETBSD_LFS;
	static const uuid_t nb_raid = GPT_ENT_TYPE_NETBSD_RAIDFRAME;
	static const uuid_t nb_ccd = GPT_ENT_TYPE_NETBSD_CCD;
	static const uuid_t nb_cgd = GPT_ENT_TYPE_NETBSD_CGD;
	static char buf[80];
	char *s;

	if (show_uuid)
		goto unfriendly;

	if (uuid_equal(t, &efi_slice, NULL))
		return ("EFI System");
	if (uuid_equal(t, &bios_boot, NULL))
		return ("BIOS Boot");
	if (uuid_equal(t, &nb_swap, NULL))
		return ("NetBSD swap");
	if (uuid_equal(t, &nb_ffs, NULL))
		return ("NetBSD FFSv1/FFSv2");
	if (uuid_equal(t, &nb_lfs, NULL))
		return ("NetBSD LFS");
	if (uuid_equal(t, &nb_raid, NULL))
		return ("NetBSD RAIDFrame component");
	if (uuid_equal(t, &nb_ccd, NULL))
		return ("NetBSD ccd component");
	if (uuid_equal(t, &nb_cgd, NULL))
		return ("NetBSD Cryptographic Disk");
	if (uuid_equal(t, &swap, NULL))
		return ("FreeBSD swap");
	if (uuid_equal(t, &ufs, NULL))
		return ("FreeBSD UFS/UFS2");
	if (uuid_equal(t, &vinum, NULL))
		return ("FreeBSD vinum");
	if (uuid_equal(t, &zfs, NULL))
		return ("FreeBSD ZFS");
	if (uuid_equal(t, &freebsd, NULL))
		return ("FreeBSD legacy");
	if (uuid_equal(t, &msdata, NULL))
		return ("Windows basic data");
	if (uuid_equal(t, &msr, NULL))
		return ("Windows reserved");
	if (uuid_equal(t, &linuxdata, NULL))
		return ("Linux data");
	if (uuid_equal(t, &linuxswap, NULL))
		return ("Linux swap");
	if (uuid_equal(t, &hfs, NULL))
		return ("Apple HFS");

unfriendly:
	uuid_to_string(t, &s, NULL);
	strlcpy(buf, s, sizeof buf);
	free(s);
	return (buf);
}

static void
show(void)
{
	uuid_t guid, type;
	off_t start;
	map_t *m, *p;
	struct mbr *mbr;
	struct gpt_ent *ent;
	unsigned int i;
	char *s;

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
				le_uuid_dec(ent->ent_guid, &guid);
				uuid_to_string(&guid, &s, NULL);
				printf("- %s", s);
				free(s);
			} else {
				le_uuid_dec(ent->ent_type, &type);
				printf("- %s", friendly(&type));
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
	uuid_t guid, type;
	map_t *m;
	struct gpt_ent *ent;
	const char *s1;
	char *s2, human_num[5];

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
	if (humanize_number(human_num, 5, (int64_t)(m->map_start * secsz),
	    "", HN_AUTOSCALE, HN_NOSPACE|HN_B) < 0)
		human_num[0] = '\0';
	if (human_num[0] != '\0')
		printf("Start: %llu (%s)\n", (long long)m->map_start,
		    human_num);
	else
		printf("Start: %llu\n", (long long)m->map_start);
	if (humanize_number(human_num, 5, (int64_t)(m->map_size * secsz),
	    "", HN_AUTOSCALE, HN_NOSPACE|HN_B) < 0)
		human_num[0] = '\0';
	if (human_num[0] != '\0')
		printf("Size: %llu (%s)\n", (long long)m->map_size, human_num);
	else
		printf("Size: %llu\n", (long long)m->map_size);

	le_uuid_dec(ent->ent_type, &type);
	s1 = friendly(&type);
	uuid_to_string(&type, &s2, NULL);
	if (strcmp(s1, s2) == 0)
		s1 = "unknown";
	printf("Type: %s (%s)\n", s1, s2);
	free(s2);

	le_uuid_dec(ent->ent_guid, &guid);
	uuid_to_string(&guid, &s2, NULL);
	printf("GUID: %s\n", s2);
	free(s2);

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

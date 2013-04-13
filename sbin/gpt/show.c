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
__RCSID("$NetBSD: show.c,v 1.10 2013/04/13 18:32:01 jakllsch Exp $");
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

const char showmsg[] = "show [-lu] device ...";

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
show(int fd __unused)
{
	uuid_t type;
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

int
cmd_show(int argc, char *argv[])
{
	int ch, fd;

	while ((ch = getopt(argc, argv, "lu")) != -1) {
		switch(ch) {
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

		show(fd);

		gpt_close(fd);
	}

	return (0);
}

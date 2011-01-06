/*	$NetBSD: biosboot.c,v 1.1 2011/01/06 01:08:48 jakllsch Exp $ */

/*
 * Copyright (c) 2009 The NetBSD Foundation, Inc. 
 * All rights reserved.
 *     
 * This code is derived from software contributed to the NetBSD Foundation
 * by Mike M. Volokhov. Development of this software was supported by the
 * Google Summer of Code program.
 * The GSoC project was mentored by Allen Briggs and Joerg Sonnenberger.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifdef __RCSID
__RCSID("$NetBSD: biosboot.c,v 1.1 2011/01/06 01:08:48 jakllsch Exp $");
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/disk.h>
#include <sys/param.h>
#include <sys/bootblock.h>

#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <paths.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "map.h"
#include "gpt.h"

#define DEFAULT_BOOTDIR		"/usr/mdec"
#define DEFAULT_BOOTCODE	"mbr_gpt"

static daddr_t start;
static uint64_t size;

static char *bootpath;
static unsigned int entry;

const uuid_t uuid_mbr_guid_default = MBR_GPT_GUID_DEFAULT;

const char biosbootmsg[] = "biosboot [-c bootcode] [-i index] device ...";

static void
usage_biosboot(void)
{
	fprintf(stderr, "usage: %s %s\n", getprogname(), biosbootmsg);
	exit(1);
}

static struct mbr*
read_boot(void)
{
	int bfd, ret = 0;
	struct mbr *buf;
	struct stat st;
	uuid_t uuid_patch_magic;

	/* XXX how to do the following better? */
	if (bootpath == NULL) {
		bootpath = strdup(DEFAULT_BOOTDIR "/" DEFAULT_BOOTCODE);
	} else {
		if (strchr(bootpath, '/') == 0) {
			char *p;
			if ((p = strdup(bootpath)) == NULL)
				err(1, "Malloc failed");
			free(bootpath);
			(void)asprintf(&bootpath, "%s/%s", DEFAULT_BOOTDIR, p);
			free(p);
		}
	}
	if (bootpath == NULL)
		err(1, "Malloc failed");

	if ((buf = malloc((size_t)secsz)) == NULL)
		err(1, "Malloc failed");

	if ((bfd = open(bootpath, O_RDONLY)) < 0 || fstat(bfd, &st) == -1) {
		warn("%s", bootpath);
		goto fail;
	}

	if (st.st_size != secsz) {
		warnx("%s: the bootcode does not match '%s' sector size",
			bootpath, device_name);
		goto fail;
	}

	if (read(bfd, buf, secsz) != st.st_size) {
		warn("%s", bootpath);
		goto fail;
	}

	if (le32toh(buf->mbr_sig) != MBR_SIG) {
		warnx("%s: invalid MBR magic", bootpath);
		goto fail;
	}

	uuid_dec_le(&buf->mbr_code[MBR_GPT_GUID_OFFSET], &uuid_patch_magic);

	/*
	 * The loader have to contain some magic in patchable area,
	 * such we can be sure that we won't trash something important
	 */
	if (!uuid_equal(&uuid_patch_magic, &uuid_mbr_guid_default, NULL)) {
		warnx("%s: the bootcode does not support required options",
			bootpath);
		goto fail;
	}

	ret++;

    fail:
	if (bfd >= 0)
		close(bfd);
	if (ret == 0) {
		free(buf);
		buf = NULL;
	}
	return buf;
}

static void
biosboot(int fd)
{
	map_t *gpt, *tpg;
	map_t *tbl, *lbt;
	map_t *mbrmap, *m;
	struct mbr *mbr, *bootcode;
	struct gpt_ent *pp;
	uuid_t buuid;

	/*
	 * Parse and validate partition maps
	 */
	gpt = map_find(MAP_TYPE_PRI_GPT_HDR);
	if (gpt == NULL) {
		warnx("%s: error: no primary GPT header; run create or recover",
		    device_name);
		return;
	}

	tpg = map_find(MAP_TYPE_SEC_GPT_HDR);
	if (tpg == NULL) {
		warnx("%s: error: no secondary GPT header; run recover",
		    device_name);
		return;
	}

	tbl = map_find(MAP_TYPE_PRI_GPT_TBL);
	lbt = map_find(MAP_TYPE_SEC_GPT_TBL);
	if (tbl == NULL || lbt == NULL) {
		warnx("%s: error: run recover -- trust me", device_name);
		return;
	}

	mbrmap = map_find(MAP_TYPE_PMBR);
	if (mbrmap == NULL || mbrmap->map_start != 0) {
		warnx("%s: error: no valid Protective MBR found", device_name);
		return;
	}

	mbr = mbrmap->map_data;

	/*
	 * Update the boot code
	 */
	if ((bootcode = read_boot()) == NULL) {
		warnx("error reading bootcode");
		return;
	}
	(void)memcpy(&mbr->mbr_code, &bootcode->mbr_code,
		sizeof(mbr->mbr_code));
	free(bootcode);

	/*
	 * Walk through the GPT and see where we can boot from
	 */
	for (m = map_first(); m != NULL; m = m->map_next) {
		if (m->map_type != MAP_TYPE_GPT_PART || m->map_index < 1)
			continue;

		pp = m->map_data;

		/* first, prefer user selection */
		if (entry > 0 && m->map_index == entry)
			break;

		/* next, partition as could be specified by wedge */
		if (entry < 1 && size > 0 &&
		    m->map_start == start && m->map_size == (off_t)size)
			break;
	}

	if (m == NULL) {
		warnx("error: no bootable partition");
		return;
	} else
		uuid_dec_le(pp->ent_guid, &buuid);

#if 1
	{
		char *p;

		uuid_to_string(&buuid, &p, NULL);
		printf("Boot device:     %s\n", device_name);
		printf("Partition GUID:  %s\n", p);
		printf("Boot code:       %s\n", bootpath);

		free(p);
	}
#endif

	uuid_enc_le(&mbr->mbr_code[MBR_GPT_GUID_OFFSET], &buuid);
	if (gpt_write(fd, mbrmap) == -1) {
		warnx("error: cannot update Protective MBR");
		return;
	}
}

int
cmd_biosboot(int argc, char *argv[])
{
	struct dkwedge_info dkw;
	struct stat sb;
	char devpath[MAXPATHLEN];
	char *dev, *p;
	int ch, fd;

	while ((ch = getopt(argc, argv, "c:i:")) != -1) {
		switch(ch) {
		case 'c':
			if (bootpath != NULL)
				usage_biosboot();
			if ((bootpath = strdup(optarg)) == NULL)
				err(1, "Malloc failed");
			break;
		case 'i':
			if (entry > 0)
				usage_biosboot();
			entry = strtoul(optarg, &p, 10);
			if (*p != 0 || entry < 1)
				usage_biosboot();
			break;
		default:
			usage_biosboot();
		}
	}

	if (argc == optind)
		usage_biosboot();

	while (optind < argc) {
		dev = argv[optind++];
		start = 0;
		size = 0;

#ifdef __NetBSD__
		/*
		 * If a dk wedge was specified, loader should be
		 * installed onto parent device
		 */
		if ((fd = opendisk(dev, O_RDONLY, devpath, sizeof(devpath), 0))
				== -1)
			goto next;
		if (fstat(fd, &sb) == -1)
			goto close;

#ifdef DIOCGWEDGEINFO
		if ((sb.st_mode & S_IFMT) != S_IFREG &&
		    ioctl(fd, DIOCGWEDGEINFO, &dkw) != -1) {
			if (entry > 0)
				/* wedges and indexes are mutually exclusive */
				usage_biosboot();
			dev = dkw.dkw_parent;
			start = dkw.dkw_offset;
			size = dkw.dkw_size;
		}
#endif
	close:
		close(fd);
#endif	/* __NetBSD__*/

		fd = gpt_open(dev);
	next:
		if (fd == -1) {
			warn("unable to open device '%s'", device_name);
			continue;
		}

		biosboot(fd);

		gpt_close(fd);
	}

	return (0);
}

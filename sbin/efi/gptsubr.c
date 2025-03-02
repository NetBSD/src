/* $NetBSD: gptsubr.c,v 1.3 2025/03/02 00:03:41 riastradh Exp $ */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: gptsubr.c,v 1.3 2025/03/02 00:03:41 riastradh Exp $");
#endif /* not lint */

#include <sys/param.h>

#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/disklabel_gpt.h>
#include <sys/dkio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include <err.h>
#include <fcntl.h>
#include <libgen.h>
#include <paths.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <uuid.h>

#include "defs.h"
#include "gpt.h"
#include "gptsubr.h"
#include "map.h"

#define DEBUG

#ifdef DEBUG
#define DPRINTF(fmt, ...)	printf(fmt, __VA_ARGS__)
#else
#define DPRINTF(fmt, ...)	do { } while (/*CONSTCOND*/0)
#endif

static const char *
map_type_name(int map_type)
{
	const char *xtbl[] = {
		"UNUSED",	// 0
		"MBR",		// 1
		"MBR_PART",	// 2
		"PRI_GPT_HDR",	// 3
		"SEC_GPT_HDR",	// 4
		"PRI_GPT_TBL",	// 5
		"SEC_GPT_TBL",	// 6
		"GPT_PART",	// 7
		"PMBR",		// 8
	};

	if (map_type >= (int)__arraycount(xtbl))
		return "UNKNOWN";

	return xtbl[map_type];
}

#if 0
struct map {
	off_t	map_start;
	off_t	map_size;
	struct map *map_next;
	struct map *map_prev;
	int	map_type;
#define	MAP_TYPE_UNUSED		0
#define	MAP_TYPE_MBR		1
#define	MAP_TYPE_MBR_PART	2
#define	MAP_TYPE_PRI_GPT_HDR	3
#define	MAP_TYPE_SEC_GPT_HDR	4
#define	MAP_TYPE_PRI_GPT_TBL	5
#define	MAP_TYPE_SEC_GPT_TBL	6
#define	MAP_TYPE_GPT_PART	7
#define	MAP_TYPE_PMBR		8
	unsigned int map_index;
	void 	*map_data;
	int	map_alloc;
};

struct gpt_ent {
	uint8_t		ent_type[16];	/* partition type GUID */
	uint8_t		ent_guid[16];	/* unique partition GUID */
	uint64_t	ent_lba_start;	/* start of partition */
	uint64_t	ent_lba_end;	/* end of partition */
	uint64_t	ent_attr;	/* partition attributes */
	uint16_t	ent_name[36];	/* partition name in UCS-2 */
};

struct gpt_hdr {
	int8_t		hdr_sig[8];	/* identifies GUID Partition Table */
	uint32_t	hdr_revision;	/* GPT specification revision */
	uint32_t	hdr_size;	/* size of GPT Header */
	uint32_t	hdr_crc_self;	/* CRC32 of GPT Header */
	uint32_t	hdr__rsvd0;	/* must be zero */
	uint64_t	hdr_lba_self;	/* LBA that contains this Header */
	uint64_t	hdr_lba_alt;	/* LBA of backup GPT Header */
	uint64_t	hdr_lba_start;	/* first LBA usable for partitions */
	uint64_t	hdr_lba_end;	/* last LBA usable for partitions */
	uint8_t		hdr_guid[16];	/* GUID to identify the disk */
	uint64_t	hdr_lba_table;	/* first LBA of GPE array */
	uint32_t	hdr_entries;	/* number of entries in GPE array */
	uint32_t	hdr_entsz;	/* size of each GPE */
	uint32_t	hdr_crc_table;	/* CRC32 of GPE array */
	/*
	 * The remainder of the block that contains the GPT Header
	 * is reserved by EFI for future GPT Header expansion, and
	 * must be zero.
	 */
};
#endif

struct map_widths {
	uint start;
	uint size;
	uint type_name;
};

static struct map_widths
get_map_widths(gpt_t gpt)
{
	struct map_widths w;
	off_t max_start = 0;
	off_t max_size = 0;
	map_t m;

	w.type_name = 0;
	for (m = map_first(gpt); m != NULL; m = m->map_next) {
		uint n;

		if (max_start < m->map_start)
			max_start = m->map_start;
		if (max_size < m->map_size)
			max_size = m->map_size;
		if (m->map_type == MAP_TYPE_GPT_PART) {
			struct gpt_ent *ent = m->map_data;
			gpt_uuid_t gpt_uuid;
			char ent_type[128];

			memcpy(&gpt_uuid, ent->ent_type, sizeof(gpt_uuid));
			n = (uint)gpt_uuid_snprintf(ent_type, sizeof(ent_type), "%s", gpt_uuid);
			if (w.type_name < n)
				w.type_name = n;
		}

	}
	w.start = (uint)snprintf(NULL, 0, "%" PRIx64, max_start);
	w.size = (uint)snprintf(NULL, 0, "%" PRIx64, max_size);

#define MIN_WIDTH_OF(s)	(sizeof(s) - 1)
	if (w.type_name < MIN_WIDTH_OF("TYPE_NAME"))
		w.type_name = MIN_WIDTH_OF("TYPE_NAME");
	if (w.start < MIN_WIDTH_OF("START"))
		w.start = MIN_WIDTH_OF("START");
	if (w.size < MIN_WIDTH_OF("SIZE"))
		w.size = MIN_WIDTH_OF("SIZE");
#undef MIN_WIDTH_OF

	return w;
}

static void
show_map_banner(struct map_widths w)
{

	printf("IDX %*s %*s   ATTR            PARTITION_GUID"
	    "                         TYPE_UUID       %*s "
	    "TYPE_NAME ENTRY_NAME  DESCRIPTION\n",
	    w.start, "START",
	    w.size, "SIZE",
	    w.type_name, "");
}

static void
show_map(map_t m, struct map_widths w)
{
	struct gpt_ent *ent;
	struct gpt_hdr *hdr;
	gpt_uuid_t gpt_uuid;
	uuid_t uuid;
	uint32_t status;
	char *type_uuid;
	char *part_guid;
	uint64_t ent_attr;
	uint8_t  ent_desc[128];
	char     ent_type[128];

	ent_desc[0] = '\0';
	ent_type[0] = '\0';
	ent_attr = 0;

	switch (m->map_type) {
	case MAP_TYPE_PRI_GPT_HDR:
	case MAP_TYPE_SEC_GPT_HDR:
		hdr = m->map_data;
		memcpy(&uuid, hdr->hdr_guid, sizeof(uuid));
		uuid_to_string(&uuid, &part_guid, &status);
		type_uuid = estrdup("");
		break;

	case MAP_TYPE_GPT_PART:
		ent = m->map_data;

		memcpy(&uuid, ent->ent_type, sizeof(uuid));
		uuid_to_string(&uuid, &type_uuid, &status);

		memcpy(&uuid, ent->ent_guid, sizeof(uuid));
		uuid_to_string(&uuid, &part_guid, &status);

		ent_attr = ent->ent_attr;

		memcpy(&gpt_uuid, ent->ent_type, sizeof(uuid));
		gpt_uuid_snprintf(ent_type, sizeof(ent_type), "%s", gpt_uuid);

		/*
		 * Use the gpt.c code here rather than our
		 * ucs2_to_utf8() as we are in their world.
		 */
		utf16_to_utf8(ent->ent_name, sizeof(ent->ent_name),
		    ent_desc, sizeof(ent_desc));

		break;

	case MAP_TYPE_MBR_PART:
		part_guid = estrdup("");
		type_uuid = estrdup("");
		break;

	case MAP_TYPE_MBR: {
		struct mbr *mbr = m->map_data;

		easprintf(&part_guid, "%02x%02x%02x%02x-0000-0000-0000-000000000000",
		    mbr->mbr_code[440],
		    mbr->mbr_code[441],
		    mbr->mbr_code[442],
		    mbr->mbr_code[443]);
		uuid_to_string(&(uuid_t)GPT_ENT_TYPE_MBR, &type_uuid, &status);
		break;
	}
	default:
		part_guid = estrdup("");
		type_uuid = estrdup("");
		break;
	}

	printf("%2u: %*" PRIx64 " %*" PRIx64 " %08" PRIx64
	    " %36s %36s %*s %-11s %s\n",
	    m->map_index,
	    w.start,
	    m->map_start,
	    w.size,
	    m->map_size,
	    ent_attr,
	    part_guid,
	    type_uuid,
	    w.type_name,
	    ent_type,
	    map_type_name(m->map_type),
	    ent_desc);

	free(part_guid);
	free(type_uuid);
}

PUBLIC map_t
find_gpt_map(const char *dev, uint idx)
{
	gpt_t gpt;
	map_t m;

	gpt = gpt_open(dev, GPT_READONLY, 0, 0, 0, 0);

	if (gpt == NULL)
		err(EXIT_FAILURE, "gpt_open");

	if (map_find(gpt, MAP_TYPE_PRI_GPT_HDR) == NULL)
		printf("GPT not found, displaying data from MBR.\n");

	for (m = map_first(gpt); m != NULL; m = m->map_next) {
		if (m->map_index == idx)
			break;
	}
	gpt_close(gpt);
	return m;
}

PUBLIC char *
parent_of_fname(const char *fname)
{
	struct dkwedge_info dkinfo;
	struct statvfs vfsb;
	struct stat sb;
	const char *d, *b;
	char *p;
	size_t n;
	int fd, rv;

	rv = stat(fname, &sb);
	if (rv == -1)
		err(EXIT_FAILURE, "stat: %s", fname);

	if (S_ISBLK(sb.st_mode) || S_ISCHR(sb.st_mode))
		return estrdup(fname);

	rv = statvfs(fname, &vfsb);
	if (rv == -1)
		err(EXIT_FAILURE, "statvfs: %s", fname);

	b = basename(vfsb.f_mntfromname);
	d = dirname(vfsb.f_mntfromname);
	easprintf(&p, "%s/r%s", d, b);

	fd = open(p, O_RDONLY);
	if (fd == -1)
		err(EXIT_FAILURE, "open");

	rv = ioctl(fd, DIOCGWEDGEINFO, &dkinfo);
	close(fd);

	if (rv != -1) {
		free(p);
		return estrdup(dkinfo.dkw_parent);
	}

	warn("ioctl: DIOCGWEDGEINFO");

	/*
	 * Hum.  No wedges?  Assume we have the old disklabel
	 * "/dev/rwd0x" syntax.  Convert it to "/dev/rwd0d".
	 * XXX: this probably won't work.
	 */
	n = strlen(p);
	p[n - 1] = 'd';

	return p;
}

PUBLIC int
mbr_sig_write(const char *fname, uint32_t new_sig, bool force, int verbose)
{
	gpt_t gpt;
	map_t m;
	struct mbr *mbr;
	const char *dev;
	uint32_t old_sig;

	if (fname == NULL)
		errx(EXIT_FAILURE, "please specify a device");

	dev = parent_of_fname(fname);
	if (dev == NULL) {
		warnx("unable to find parent device of %s\n", fname);
		return -1;
	}

	gpt = gpt_open(dev, GPT_MODIFIED, verbose, 0, 0, 0);

	if (gpt == NULL)
		err(EXIT_FAILURE, "gpt_open");

	m = map_find(gpt, MAP_TYPE_MBR);
	if (m == NULL)
		printf("No MBR partition found!\n");
	else {
		mbr = m->map_data;

		memcpy(&old_sig, &mbr->mbr_code[440], 4);

		if (old_sig == 0)
			force = true;

		if (force) {
			memcpy(&mbr->mbr_code[440], &new_sig, 4);
			if (gpt_write(gpt, m) == -1)
				warn("gpt_write");
			else if (verbose)
				printf("sig: 0x%08x -> 0x%08x\n",
				    old_sig, new_sig);
		}
		else if (verbose)
			printf("sig: 0x%08x (unchanged)\n", old_sig);
	}

	gpt_close(gpt);
	return 0;
}

PUBLIC int
show_gpt(const char *fname, int verbose)
{
	gpt_t gpt;
	map_t m;
	struct map_widths w;
	const char *dev;

	dev = parent_of_fname(fname);
	if (dev == NULL)
		return -1;

	gpt = gpt_open(dev, GPT_READONLY, verbose, 0, 0, 0);

	if (gpt == NULL)
		err(EXIT_FAILURE, "gpt_open");

	if (map_find(gpt, MAP_TYPE_PRI_GPT_HDR) == NULL)
		warnx("GPT not found, displaying data from MBR.");

	w = get_map_widths(gpt);
	show_map_banner(w);

	for (m = map_first(gpt); m != NULL; m = m->map_next)
		show_map(m, w);

	gpt_close(gpt);
	return 0;
}

#if 0 /* UNUSED */

PUBLIC char *
wedge_of_fname(const char *fname)
{
	struct statvfs vfsbuf;
	int rv;

	rv = statvfs(fname, &vfsbuf);
	if (rv) {
		warn("statvfs: %s", fname);
		return NULL;
	}

	return estrdup(vfsbuf.f_mntfromname);
}

PUBLIC int
find_partition_idx(const char *fname, int verbose)
{
	struct dkwedge_info dkinfo;
	struct stat sb;
	struct statvfs vfsbuf;
	gpt_t gpt;
	map_t m;
	off_t offset;
	size_t size;
	const char *parent;
	char *b, *d;
	int rv;

	/* the following are for gpt_open() */
	off_t mediasz = 0;
	u_int secsz = 0;
	time_t timestamp = 0;

	rv = stat(fname, &sb);
	if (rv == -1)
		err(EXIT_FAILURE, "stat: %s", fname);

	if (S_ISBLK(sb.st_mode) || S_ISCHR(sb.st_mode)) {
		parent = fname;
		offset = 0;
		size = 0;
		goto doit;
	}

	rv = statvfs(fname, &vfsbuf);
	if (rv == -1)
		err(EXIT_FAILURE, "statvfs: %s", fname);

	b = basename(vfsbuf.f_mntfromname);
	d = dirname(vfsbuf.f_mntfromname);

	{
		char *p;

		easprintf(&p, "%s/r%s", d, b);

		int fd = open(p, O_RDONLY);
		if (fd == -1)
			err(EXIT_FAILURE, "open");

		rv = ioctl(fd, DIOCGWEDGEINFO, &dkinfo);
		if (rv != -1) {
			parent = dkinfo.dkw_parent;
			offset = dkinfo.dkw_offset;
			size   = dkinfo.dkw_size;
		}
		else {
			struct disklabel dl;

			warn("ioctl: DIOCGWEDGEINFO");

			rv = ioctl(fd, DIOCGDINFO, &dl);
			if (rv == -1)
				err(EXIT_FAILURE, "ioctl: DIOCGDINFO");

			size_t n = strlen(p);

			int pnum = p[n - 1] - 'a';
			p[n - 1] = 'd';

			printf("num_parts: %u\n", dl.d_npartitions);
			printf("partition %d\n", pnum);
			printf("  offset = %u (%#x)\n", dl.d_partitions[pnum].p_offset, dl.d_partitions[pnum].p_offset);
			printf("  size = %u (%#x)\n",   dl.d_partitions[pnum].p_size,   dl.d_partitions[pnum].p_size);

			parent = p;	// vfsbuf.f_mntfromname;
			offset = dl.d_partitions[pnum].p_offset;
			size   = dl.d_partitions[pnum].p_size;
		}

		close(fd);
		free(p);
	}

 doit:
	DPRINTF("parent: %s\n", parent);
	DPRINTF("offset: 0x%lx\n", offset);
	DPRINTF("size: 0x%lx\n", size);

	gpt = gpt_open(parent, GPT_READONLY,
	    verbose, mediasz, secsz, timestamp);

	if (gpt == NULL)
		err(EXIT_FAILURE, "gpt_open");

	if (map_find(gpt, MAP_TYPE_PRI_GPT_HDR) == NULL)
		printf("GPT not found, displaying data from MBR.\n");

	int index = -1;
	struct map_widths w;
	if (verbose) {
		w = get_map_widths(gpt);
		show_map_banner(w);
	}
	for (m = map_first(gpt); m != NULL; m = m->map_next) {
		if (verbose)
			show_map(m, w);

		if (m->map_type != MAP_TYPE_GPT_PART || m->map_index < 1) {
			continue;
		}

		if ((off_t)offset == m->map_start &&
		    (off_t)size   == m->map_size) {
			if (index != -1)
				printf("match: %u\n", index);
			index = (int)m->map_index;
		}
	}
	return index;
}

PUBLIC char *
find_partition_pathname(const char *fname)
{
	char *pname;	/* partition name */
	char *rname;	/* real name */
	struct statvfs vfsbuf;
	int i, rv;

	rname = realpath(fname, NULL);

	DPRINTF("fname: %s\n", fname);
	DPRINTF("rname: %s\n", rname);

	rv = statvfs(rname, &vfsbuf);
	if (rv) {
		warn("statvfs: %s", rname);
		free(rname);
		return NULL;
	}

	DPRINTF("mount: %s\n", vfsbuf.f_mntonname);

	i = 0;
	while (vfsbuf.f_mntonname[i] == rname[i] && vfsbuf.f_mntonname[i] != '\0')
		i++;

	if (vfsbuf.f_mntonname[i] != '\0')
		errx(EXIT_FAILURE, "mntonname mismatch: %s",
		    vfsbuf.f_mntonname + i);

	pname = estrdup(rname + i);
	free(rname);

	return pname;
}

#endif

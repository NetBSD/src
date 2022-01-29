/*	$NetBSD: gpt.c,v 1.27 2022/01/29 15:32:49 martin Exp $	*/

/*
 * Copyright 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "defs.h"
#include "mbr.h"
#include "md.h"
#include "gpt_uuid.h"
#include <assert.h>
#include <errno.h>
#include <err.h>
#include <paths.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <util.h>
#include <uuid.h>

bool	gpt_parts_check(void);	/* check for needed binaries */


/*************** GPT ************************************************/
/* a GPT based disk_partitions interface */

#define GUID_STR_LEN	40
#define	GPT_PTYPE_ALLOC	32	/* initial type array allocation, should be >
				 * gpt type -l | wc -l */
#define	GPT_DEV_LEN	16	/* dkNN */

#define	GPT_PARTS_PER_SEC	4	/* a 512 byte sector holds 4 entries */
#define	GPT_DEFAULT_MAX_PARTS	128

/* a usable label will be short, so we can get away with an arbitrary limit */
#define	GPT_LABEL_LEN		96

#define	GPT_ATTR_BIOSBOOT	1
#define	GPT_ATTR_BOOTME		2
#define	GPT_ATTR_BOOTONCE	4
#define	GPT_ATTR_BOOTFAILED	8
#define	GPT_ATTR_NOBLOCKIO	16
#define	GPT_ATTR_REQUIRED	32

/* when we don't care for BIOS or UEFI boot, use the combined boot flags */
#define	GPT_ATTR_BOOT	(GPT_ATTR_BIOSBOOT|GPT_ATTR_BOOTME)

struct gpt_attr_desc {
	const char *name;
	uint flag;
};
static const struct gpt_attr_desc gpt_avail_attrs[] = {
	{ "biosboot", GPT_ATTR_BIOSBOOT },
	{ "bootme", GPT_ATTR_BOOTME },
	{ "bootonce", GPT_ATTR_BOOTONCE },
	{ "bootfailed", GPT_ATTR_BOOTFAILED },
	{ "noblockio", GPT_ATTR_NOBLOCKIO },
	{ "required", GPT_ATTR_REQUIRED },
	{ NULL, 0 }
};

struct gpt_ptype_desc {
	struct part_type_desc gent;
	char tid[GUID_STR_LEN];
	uint fsflags, default_fs_type;
};

static const
struct {
	const char *name;
	uint fstype;
	enum part_type ptype;
	uint fsflags;
} gpt_fs_types[] = {
	{ .name = "ffs",	.fstype = FS_BSDFFS,	.ptype = PT_root,
	  .fsflags = GLM_LIKELY_FFS },
	{ .name = "swap",	.fstype = FS_SWAP,	.ptype = PT_swap },
	{ .name = "windows",	.fstype = FS_MSDOS,	.ptype = PT_FAT,
	  .fsflags = GLM_MAYBE_FAT32|GLM_MAYBE_NTFS },
	{ .name = "windows",	.fstype = FS_NTFS,	.ptype = PT_FAT,
	  .fsflags = GLM_MAYBE_FAT32|GLM_MAYBE_NTFS },
	{ .name = "efi",	.fstype = FS_MSDOS,	.ptype = PT_EFI_SYSTEM,
	  .fsflags = GLM_MAYBE_FAT32 },
	{ .name = "bios",	.fstype = FS_MSDOS,	.ptype = PT_FAT,
	  .fsflags = GLM_MAYBE_FAT32 },
	{ .name = "lfs",	.fstype = FS_BSDLFS,	.ptype = PT_root },
	{ .name = "linux-data",	.fstype = FS_EX2FS,	.ptype = PT_root },
	{ .name = "apple",	.fstype = FS_HFS,	.ptype = PT_unknown },
	{ .name = "ccd",	.fstype = FS_CCD,	.ptype = PT_root },
	{ .name = "cgd",	.fstype = FS_CGD,	.ptype = PT_root },
	{ .name = "raid",	.fstype = FS_RAID,	.ptype = PT_root },
	{ .name = "vmcore",	.fstype = FS_VMKCORE,	.ptype = PT_unknown },
	{ .name = "vmfs",	.fstype = FS_VMFS,	.ptype = PT_unknown },
	{ .name = "vmresered",	.fstype = FS_VMWRESV,	.ptype = PT_unknown },
	{ .name = "zfs",	.fstype = FS_ZFS,	.ptype = PT_root },
};

static size_t gpt_ptype_cnt = 0, gpt_ptype_alloc = 0;
static struct gpt_ptype_desc *gpt_ptype_descs = NULL;

/* "well" known types with special handling */
static const struct part_type_desc *gpt_native_root;

/* similar to struct gpt_ent, but matching our needs */
struct gpt_part_entry {
	const struct gpt_ptype_desc *gp_type;
	char gp_id[GUID_STR_LEN];	/* partition guid as string */
	daddr_t gp_start, gp_size;
	uint gp_attr;			/* various attribute bits */
	char gp_label[GPT_LABEL_LEN];	/* user defined label */
	char gp_dev_name[GPT_DEV_LEN];	/* name of wedge */
	const char *last_mounted;	/* last mounted if known */
	uint fs_type, fs_sub_type,	/* FS_* and maybe sub type */
	    fs_opt1, fs_opt2, fs_opt3;	/* transient file system options */
	uint gp_flags;
#define	GPEF_ON_DISK	1		/* This entry exists on-disk */
#define	GPEF_MODIFIED	2		/* this entry has been changed */
#define	GPEF_WEDGE	4		/* wedge for this exists */
#define	GPEF_RESIZED	8		/* size has changed */
#define	GPEF_TARGET	16		/* marked install target */
	struct gpt_part_entry *gp_next;
};

static const struct gpt_ptype_desc *gpt_find_native_type(
    const struct part_type_desc *gent);
static const struct gpt_ptype_desc *gpt_find_guid_type(const char*);
static bool
gpt_info_to_part(struct gpt_part_entry *p, const struct disk_part_info *info,
    const char **err_msg);

const struct disk_partitioning_scheme gpt_parts;
struct gpt_disk_partitions {
	struct disk_partitions dp;
	/*
	 * We keep a list of our current valid partitions, pointed
	 * to by "partitions".
	 * dp.num_part is the number of entries in "partitions".
	 * When partitions that have a representation on disk already
	 * are deleted, we move them to the "obsolete" list so we
	 * can issue the proper commands to remove it when writing back.
	 */
	struct gpt_part_entry *partitions,	/* current partitions */
	    *obsolete;				/* deleted partitions */
	size_t max_num_parts;			/* how many entries max? */
	size_t prologue, epilogue;		/* number of sectors res. */
	bool has_gpt;	/* disk already has a GPT */
};

/*
 * Init global variables from MD details
 */
static void
gpt_md_init(bool is_boot_disk, size_t *max_parts, size_t *head, size_t *tail)
{
	size_t num;

	if (is_boot_disk) {
#ifdef MD_GPT_INITIAL_SIZE
#if MD_GPT_INITIAL_SIZE < 2*512
#error	impossible small GPT prologue
#endif
		num = ((MD_GPT_INITIAL_SIZE-(2*512))/512)*GPT_PARTS_PER_SEC;
#else
		num = GPT_DEFAULT_MAX_PARTS;
#endif
	} else {
		num = GPT_DEFAULT_MAX_PARTS;
	}
	*max_parts = num;
	*head = 2 + num/GPT_PARTS_PER_SEC;
	*tail = 1 + num/GPT_PARTS_PER_SEC;
}

/*
 * Parse a part of "gpt show" output into a struct gpt_part_entry.
 * Output is from "show -a" format if details = false, otherwise
 * from details for a specific partition (show -i or show -b)
 */
static void
gpt_add_info(struct gpt_part_entry *part, const char *tag, char *val,
    bool details)
{
	char *s, *e;

	if (details && strcmp(tag, "Start:") == 0) {
		part->gp_start = strtouq(val, NULL, 10);
	} else if (details && strcmp(tag, "Size:") == 0) {
		part->gp_size = strtouq(val, NULL, 10);
	} else if (details && strcmp(tag, "Type:") == 0) {
		s = strchr(val, '(');
		if (!s)
			return;
		e = strchr(s, ')');
		if (!e)
			return;
		*e = 0;
		part->gp_type = gpt_find_guid_type(s+1);
	} else if (strcmp(tag, "TypeID:") == 0) {
		part->gp_type = gpt_find_guid_type(val);
	} else if (strcmp(tag, "GUID:") == 0) {
		strlcpy(part->gp_id, val, sizeof(part->gp_id));
	} else if (strcmp(tag, "Label:") == 0) {
		strlcpy(part->gp_label, val, sizeof(part->gp_label));
	} else if (strcmp(tag, "Attributes:") == 0) {
		char *n;

		while ((n = strsep(&val, ", ")) != NULL) {
			if (*n == 0)
				continue;
			for (const struct gpt_attr_desc *p = gpt_avail_attrs;
			    p->name != NULL; p++) {
				if (strcmp(p->name, n) == 0)
					part->gp_attr |= p->flag;
			}
		}
	}
}

/*
 * Find the partition matching this wedge info and record that we
 * have a wedge already.
 */
static void
update_part_from_wedge_info(struct gpt_disk_partitions *parts,
    const struct dkwedge_info *dkw)
{
	for (struct gpt_part_entry *p = parts->partitions; p != NULL;
	    p = p->gp_next) {
		if (p->gp_start != dkw->dkw_offset ||
		    (uint64_t)p->gp_size != dkw->dkw_size)
			continue;
		p->gp_flags |= GPEF_WEDGE;
		strlcpy(p->gp_dev_name, dkw->dkw_devname,
		    sizeof p->gp_dev_name);
		return;
	}
}

static struct disk_partitions *
gpt_read_from_disk(const char *dev, daddr_t start, daddr_t len, size_t bps,
    const struct disk_partitioning_scheme *scheme)
{
	char diskpath[MAXPATHLEN];
	int fd;
	struct dkwedge_info *dkw;
	struct dkwedge_list dkwl;
	size_t bufsize, dk;

	assert(start == 0);
	assert(have_gpt);

	if (run_program(RUN_SILENT | RUN_ERROR_OK,
	    "gpt -rq header %s", dev) != 0)
		return NULL;

	/* read the partitions */
	int i;
	unsigned int p_index;
	daddr_t p_start = 0, p_size = 0, avail_start = 0, avail_size = 0,
	    disk_size = 0;
	char *textbuf, *t, *tt, p_type[STRSIZE];
	static const char regpart_prefix[] = "GPT part - ";
	struct gpt_disk_partitions *parts;
	struct gpt_part_entry *last = NULL, *add_to = NULL;
	const struct gpt_ptype_desc *native_root
	     = gpt_find_native_type(gpt_native_root);
	bool have_target = false;

	if (collect(T_OUTPUT, &textbuf, "gpt -r show -a %s 2>/dev/null", dev)
	    < 1)
		return NULL;

	/* parse output and create our list */
	parts = calloc(1, sizeof(*parts));
	if (parts == NULL)
		return NULL;

	(void)strtok(textbuf, "\n"); /* ignore first line */
	while ((t = strtok(NULL, "\n")) != NULL) {
		i = 0; p_start = 0; p_size = 0; p_index = 0;
		p_type[0] = 0;
		while ((tt = strsep(&t, " \t")) != NULL) {
			if (strlen(tt) == 0)
				continue;
			if (i == 0) {
				if (add_to != NULL)
					gpt_add_info(add_to, tt, t, false);
				p_start = strtouq(tt, NULL, 10);
				if (p_start == 0 && add_to != NULL)
					break;
				else
					add_to = NULL;
			}
			if (i == 1)
				p_size = strtouq(tt, NULL, 10);
			if (i == 2)
				p_index = strtouq(tt, NULL, 10);
			if (i > 2 || (i == 2 && p_index == 0)) {
				if (p_type[0])
					strlcat(p_type, " ", STRSIZE);
				strlcat(p_type, tt, STRSIZE);
			}
			i++;
		}

		if (p_start == 0 || p_size == 0)
			continue;
		else if (strcmp(p_type, "Pri GPT table") == 0) {
			avail_start = p_start + p_size;
			parts->prologue = avail_start;
			parts->epilogue = p_size + 1;
			parts->max_num_parts = p_size * GPT_PARTS_PER_SEC;
		} else if (strcmp(p_type, "Sec GPT table") == 0)
			avail_size = p_start - avail_start;
		else if(strcmp(p_type, "Sec GPT header") == 0)
			disk_size = p_start + p_size;
		else if (p_index == 0 && strlen(p_type) > 0)
			/* Utilitary entry (PMBR, etc) */
			continue;
		else if (p_index == 0) {
			/* Free space */
			continue;
		} else {
			/* Usual partition */
			tt = p_type;
			if (strncmp(tt, regpart_prefix,
			    strlen(regpart_prefix)) == 0)
				tt += strlen(regpart_prefix);

			/* Add to our linked list */
			struct gpt_part_entry *np = calloc(1, sizeof(*np));
			if (np == NULL)
				break;

			strlcpy(np->gp_label, tt, sizeof(np->gp_label));
			np->gp_start = p_start;
			np->gp_size = p_size;
			np->gp_flags |= GPEF_ON_DISK;
			if (!have_target && native_root != NULL &&
			    strcmp(np->gp_id, native_root->tid) == 0) {
				have_target = true;
				np->gp_flags |= GPEF_TARGET;
			}

			if (last == NULL)
				parts->partitions = np;
			else
				last->gp_next = np;
			last = np;
			add_to = np;
			parts->dp.num_part++;
		}
	}
	free(textbuf);

	/* If the GPT was not complete (e.g. truncated image), barf */
	if (disk_size <= 0) {
		free(parts);
		return NULL;
	}

	parts->dp.pscheme = scheme;
	parts->dp.disk = strdup(dev);
	parts->dp.disk_start = start;
	parts->dp.disk_size = disk_size;
	parts->dp.free_space = avail_size;
	parts->dp.bytes_per_sector = bps;
	parts->has_gpt = true;

	fd = opendisk(parts->dp.disk, O_RDONLY, diskpath, sizeof(diskpath), 0);
	for (struct gpt_part_entry *p = parts->partitions; p != NULL;
	    p = p->gp_next) {
#ifdef DEFAULT_UFS2
		bool fs_is_default = false;
#endif

		if (p->gp_type != NULL) {

			if (p->gp_type->fsflags != 0) {
				const char *lm = get_last_mounted(fd,
				    p->gp_start, &p->fs_type,
				    &p->fs_sub_type, p->gp_type->fsflags);
				if (lm != NULL && *lm != 0) {
					char *path = strdup(lm);
					canonicalize_last_mounted(path);
					p->last_mounted = path;
				} else {
					p->fs_type = p->gp_type->
					    default_fs_type;
#ifdef DEFAULT_UFS2
					fs_is_default = true;
#endif
				}
			} else {
				p->fs_type = p->gp_type->default_fs_type;
#ifdef DEFAULT_UFS2
				fs_is_default = true;
#endif
			}
#ifdef DEFAULT_UFS2
			if (fs_is_default && p->fs_type == FS_BSDFFS)
				p->fs_sub_type = 2;
#endif
		}

		parts->dp.free_space -= p->gp_size;
	}

	/*
	 * Check if we have any (matching/auto-configured) wedges already
	 */
	dkw = NULL;
	dkwl.dkwl_buf = dkw;
	dkwl.dkwl_bufsize = 0;
	if (ioctl(fd, DIOCLWEDGES, &dkwl) == 0) {
		/* do not even try to deal with any races at this point */
		bufsize = dkwl.dkwl_nwedges * sizeof(*dkw);
		dkw = malloc(bufsize);
		dkwl.dkwl_buf = dkw;
		dkwl.dkwl_bufsize = bufsize;
		if (dkw != NULL && ioctl(fd, DIOCLWEDGES, &dkwl) == 0) {
			for (dk = 0; dk < dkwl.dkwl_ncopied; dk++)
				update_part_from_wedge_info(parts, &dkw[dk]);
		}
		free(dkw);
	}

	close(fd);

	return &parts->dp;
}

static size_t
gpt_cyl_size(const struct disk_partitions *arg)
{
	return MEG / 512;
}

static struct disk_partitions *
gpt_create_new(const char *disk, daddr_t start, daddr_t len,
    bool is_boot_drive, struct disk_partitions *parent)
{
	struct gpt_disk_partitions *parts;
	struct disk_geom geo;

	if (start != 0) {
		assert(0);
		return NULL;
	}

	if (!get_disk_geom(disk, &geo))
		return NULL;

	parts = calloc(1, sizeof(*parts));
	if (!parts)
		return NULL;

	parts->dp.pscheme = &gpt_parts;
	parts->dp.disk = strdup(disk);

	gpt_md_init(is_boot_drive, &parts->max_num_parts, &parts->prologue,
	    &parts->epilogue);

	parts->dp.disk_start = start;
	parts->dp.disk_size = len;
	parts->dp.bytes_per_sector = geo.dg_secsize;
	parts->dp.free_space = len - start - parts->prologue - parts->epilogue;
	parts->has_gpt = false;

	return &parts->dp;
}

static bool
gpt_get_part_info(const struct disk_partitions *arg, part_id id,
    struct disk_part_info *info)
{
	static const struct part_type_desc gpt_unknown_type =
		{ .generic_ptype = PT_undef,
		  .short_desc = "<unknown>" };
	const struct gpt_disk_partitions *parts =
	    (const struct gpt_disk_partitions*)arg;
	const struct gpt_part_entry *p = parts->partitions;
	part_id no;

	for (no = 0; p != NULL && no < id; no++)
		p = p->gp_next;

	if (no != id || p == NULL)
		return false;

	memset(info, 0, sizeof(*info));
	info->start = p->gp_start;
	info->size = p->gp_size;
	if (p->gp_type)
		info->nat_type = &p->gp_type->gent;
	else
		info->nat_type = &gpt_unknown_type;
	info->last_mounted = p->last_mounted;
	info->fs_type = p->fs_type;
	info->fs_sub_type = p->fs_sub_type;
	info->fs_opt1 = p->fs_opt1;
	info->fs_opt2 = p->fs_opt2;
	info->fs_opt3 = p->fs_opt3;
	if (p->gp_flags & GPEF_TARGET)
		info->flags |= PTI_INSTALL_TARGET;

	return true;
}

static bool
gpt_get_part_attr_str(const struct disk_partitions *arg, part_id id,
    char *str, size_t avail_space)
{
	const struct gpt_disk_partitions *parts =
	    (const struct gpt_disk_partitions*)arg;
	const struct gpt_part_entry *p = parts->partitions;
	part_id no;
	static const char *flags = NULL;

	for (no = 0; p != NULL && no < id; no++)
		p = p->gp_next;

	if (no != id || p == NULL)
		return false;

	if (flags == NULL)
		flags = msg_string(MSG_gpt_flags);

	if (avail_space < 2)
		return false;

	if (p->gp_attr & GPT_ATTR_BOOT)
		*str++ = flags[0];
	*str = 0;

	return true;
}

/*
 * Find insert position and check for duplicates.
 * If all goes well, insert the new "entry" in the "list".
 * If there are collisions, report "no free space".
 * We keep all lists sorted by start sector number,
 */
static bool
gpt_insert_part_into_list(struct gpt_disk_partitions *parts,
    struct gpt_part_entry **list,
    struct gpt_part_entry *entry, const char **err_msg)
{
	struct gpt_part_entry *p, *last;

	/* find the first entry past the new one (if any) */
	for (last = NULL, p = *list; p != NULL; last = p, p = p->gp_next) {
		if (p->gp_start > entry->gp_start)
			break;
	}

	/* check if last partition overlaps with new one */
	if (last) {
		if (last->gp_start + last->gp_size > entry->gp_start) {
			if (err_msg)
				*err_msg = msg_string(MSG_No_free_space);
			return false;
		}
	}

	if (p == NULL) {
		entry->gp_next = NULL;
		if (last != NULL) {
			last->gp_next = entry;
		}
	} else {
		/* check if new entry overlaps with next */
		if (entry->gp_start + entry->gp_size > p->gp_start) {
			if (err_msg)
				*err_msg = msg_string(MSG_No_free_space);
			return false;
		}

		entry->gp_next = p;
		if (last != NULL)
			last->gp_next = entry;
		else
			*list = entry;
	}
	if (*list == NULL)
		*list = entry;

	return true;
}

static bool
gpt_set_part_info(struct disk_partitions *arg, part_id id,
    const struct disk_part_info *info, const char **err_msg)
{
	struct gpt_disk_partitions *parts =
	    (struct gpt_disk_partitions*)arg;
	struct gpt_part_entry *p = parts->partitions, *n;
	part_id no;
	daddr_t lendiff;
	bool was_target;

	for (no = 0; p != NULL && no < id; no++)
		p = p->gp_next;

	if (no != id || p == NULL)
		return false;

	/* update target mark - we can only have one */
	was_target = (p->gp_flags & GPEF_TARGET) != 0;
	if (info->flags & PTI_INSTALL_TARGET)
		p->gp_flags |= GPEF_TARGET;
	else
		p->gp_flags &= ~GPEF_TARGET;
	if (was_target)
		for (n = parts->partitions; n != NULL; n = n->gp_next)
			if (n != p)
				n->gp_flags &= ~GPEF_TARGET;

	if ((p->gp_flags & GPEF_ON_DISK)) {
		if (info->start != p->gp_start) {
			/* partition moved, we need to delete and re-add */
			n = calloc(1, sizeof(*n));
			if (n == NULL) {
				if (err_msg)
					*err_msg = err_outofmem;
				return false;
			}
			*n = *p;
			p->gp_flags &= ~GPEF_ON_DISK;
			if (!gpt_insert_part_into_list(parts, &parts->obsolete,
			    n, err_msg))
				return false;
		} else if (info->size != p->gp_size) {
			p->gp_flags |= GPEF_RESIZED;
		}
	}

	p->gp_flags |= GPEF_MODIFIED;

	lendiff = info->size - p->gp_size;
	parts->dp.free_space -= lendiff;
	return gpt_info_to_part(p, info, err_msg);
}

static size_t
gpt_get_free_spaces_internal(const struct gpt_disk_partitions *parts,
    struct disk_part_free_space *result, size_t max_num_result,
    daddr_t min_space_size, daddr_t align, daddr_t start, daddr_t ignore)
{
	size_t cnt = 0;
	daddr_t s, e, from, size, end_of_disk;
	struct gpt_part_entry *p;

	if (align > 1)
		start = max(roundup(start, align), align);
	if (start < 0 || start < (daddr_t)parts->prologue)
		start = parts->prologue;
	if (parts->dp.disk_start != 0 && parts->dp.disk_start > start)
		start = parts->dp.disk_start;
	if (min_space_size < 1)
		min_space_size = 1;
	end_of_disk = parts->dp.disk_start + parts->dp.disk_size
	    - parts->epilogue;
	from = start;
	while (from < end_of_disk && cnt < max_num_result) {
again:
		size = parts->dp.disk_start + parts->dp.disk_size - from;
		start = from;
		if (start + size > end_of_disk)
			size = end_of_disk - start;
		for (p = parts->partitions; p != NULL; p = p->gp_next) {
			s = p->gp_start;
			e = p->gp_size + s;
			if (s == ignore)
				continue;
			if (e < from)
				continue;
			if (s <= from && e > from) {
				if (e - 1 >= end_of_disk)
					return cnt;
				from = e + 1;
				if (align > 1) {
					from = max(roundup(from, align), align);
					if (from >= end_of_disk) {
						size = 0;
						break;
					}
				}
				goto again;
			}
			if (s > from && s - from < size) {
				size = s - from;
			}
		}
		if (size >= min_space_size) {
			result->start = start;
			result->size = size;
			result++;
			cnt++;
		}
		from += size + 1;
		if (align > 1)
			from = max(roundup(from, align), align);
	}

	return cnt;
}

static daddr_t
gpt_max_free_space_at(const struct disk_partitions *arg, daddr_t start)
{
	const struct gpt_disk_partitions *parts =
	    (const struct gpt_disk_partitions*)arg;
	struct disk_part_free_space space;

	if (gpt_get_free_spaces_internal(parts, &space, 1, 1, 0,
	    start, start) == 1)
		return space.size;

	return 0;
}

static size_t
gpt_get_free_spaces(const struct disk_partitions *arg,
    struct disk_part_free_space *result, size_t max_num_result,
    daddr_t min_space_size, daddr_t align, daddr_t start,
    daddr_t ignore)
{
	const struct gpt_disk_partitions *parts =
	    (const struct gpt_disk_partitions*)arg;

	return gpt_get_free_spaces_internal(parts, result,
	    max_num_result, min_space_size, align, start, ignore);
}

static void
gpt_match_ptype(const char *name, struct gpt_ptype_desc *t)
{
	size_t i;

	for (i = 0; i < __arraycount(gpt_fs_types); i++) {
		if (strcmp(name, gpt_fs_types[i].name) == 0) {
			t->gent.generic_ptype = gpt_fs_types[i].ptype;
			t->fsflags = gpt_fs_types[i].fsflags;
			t->default_fs_type = gpt_fs_types[i].fstype;

			/* recongnize special entries */
			if (gpt_native_root == NULL && i == 0)
				gpt_native_root = &t->gent;

			return;
		}
	}

	t->gent.generic_ptype = PT_unknown;
	t->fsflags = 0;
	t->default_fs_type = FS_BSDFFS;
}

static void
gpt_internal_add_ptype(const char *uid, const char *name, const char *desc)
{
	if (gpt_ptype_cnt >= gpt_ptype_alloc) {
		gpt_ptype_alloc = gpt_ptype_alloc ? 2*gpt_ptype_alloc
		    : GPT_PTYPE_ALLOC;
		struct gpt_ptype_desc *nptypes = realloc(gpt_ptype_descs,
		    gpt_ptype_alloc*sizeof(*gpt_ptype_descs));
		if (nptypes == 0)
			errx(EXIT_FAILURE, "out of memory");
		gpt_ptype_descs = nptypes;
	}

	strlcpy(gpt_ptype_descs[gpt_ptype_cnt].tid, uid,
	    sizeof(gpt_ptype_descs[gpt_ptype_cnt].tid));
	gpt_ptype_descs[gpt_ptype_cnt].gent.short_desc = strdup(name);
	gpt_ptype_descs[gpt_ptype_cnt].gent.description = strdup(desc);
	gpt_match_ptype(name, &gpt_ptype_descs[gpt_ptype_cnt]);
	gpt_ptype_cnt++;
}

static void
gpt_init_ptypes(void)
{
	if (gpt_ptype_cnt == 0)
		gpt_uuid_query(gpt_internal_add_ptype);
}

static void
gpt_cleanup(void)
{
	/* free all of gpt_ptype_descs */
	for (size_t i = 0; i < gpt_ptype_cnt; i++) {
		free(__UNCONST(gpt_ptype_descs[i].gent.short_desc));
		free(__UNCONST(gpt_ptype_descs[i].gent.description));
	}
	free(gpt_ptype_descs);
	gpt_ptype_descs = NULL;
	gpt_ptype_cnt = gpt_ptype_alloc = 0;
}

static size_t
gpt_type_count(void)
{
	if (gpt_ptype_cnt == 0)
		gpt_init_ptypes();

	return gpt_ptype_cnt;
}

static const struct part_type_desc *
gpt_get_ptype(size_t ndx)
{
	if (gpt_ptype_cnt == 0)
		gpt_init_ptypes();

	if (ndx >= gpt_ptype_cnt)
		return NULL;

	return &gpt_ptype_descs[ndx].gent;
}

static const struct part_type_desc *
gpt_get_generic_type(enum part_type gent)
{
	if (gpt_ptype_cnt == 0)
		gpt_init_ptypes();

	if (gent == PT_root)
		return gpt_native_root;
	if (gent == PT_unknown)
		return NULL;

	for (size_t i = 0; i < gpt_ptype_cnt; i++)
		if (gpt_ptype_descs[i].gent.generic_ptype == gent)
			return &gpt_ptype_descs[i].gent;

	return NULL;
}

static const struct gpt_ptype_desc *
gpt_find_native_type(const struct part_type_desc *gent)
{
	if (gpt_ptype_cnt == 0)
		gpt_init_ptypes();

	if (gent == NULL)
		return NULL;

	for (size_t i = 0; i < gpt_ptype_cnt; i++)
		if (gent == &gpt_ptype_descs[i].gent)
			return &gpt_ptype_descs[i];

	gent = gpt_get_generic_type(gent->generic_ptype);
	if (gent == NULL)
		return NULL;

	/* this can not recurse deeper than once, we would not have found a
	 * generic type a few lines above if it would. */
	return gpt_find_native_type(gent);
}

static const struct gpt_ptype_desc *
gpt_find_guid_type(const char *uid)
{
	if (gpt_ptype_cnt == 0)
		gpt_init_ptypes();

	if (uid == NULL || uid[0] == 0)
		return NULL;

	for (size_t i = 0; i < gpt_ptype_cnt; i++)
		if (strcmp(gpt_ptype_descs[i].tid, uid) == 0)
			return &gpt_ptype_descs[i];

	return NULL;
}

static const struct part_type_desc *
gpt_find_type(const char *desc)
{
	if (gpt_ptype_cnt == 0)
		gpt_init_ptypes();

	if (desc == NULL || desc[0] == 0)
		return NULL;

	for (size_t i = 0; i < gpt_ptype_cnt; i++)
		if (strcmp(gpt_ptype_descs[i].gent.short_desc, desc) == 0)
			return &gpt_ptype_descs[i].gent;

	return NULL;
}

static const struct part_type_desc *
gpt_get_fs_part_type(enum part_type pt, unsigned fstype, unsigned fs_sub_type)
{
	size_t i;

	/* Try with complete match (including part_type) first */
	for (i = 0; i < __arraycount(gpt_fs_types); i++)
		if (fstype == gpt_fs_types[i].fstype &&
		    pt == gpt_fs_types[i].ptype)
			return gpt_find_type(gpt_fs_types[i].name);

	/* If that did not work, ignore part_type */
	for (i = 0; i < __arraycount(gpt_fs_types); i++)
		if (fstype == gpt_fs_types[i].fstype)
			return gpt_find_type(gpt_fs_types[i].name);

	return NULL;
}

static bool
gpt_get_default_fstype(const struct part_type_desc *nat_type,
    unsigned *fstype, unsigned *fs_sub_type)
{
	const struct gpt_ptype_desc *gtype;

	gtype = gpt_find_native_type(nat_type);
	if (gtype == NULL)
		return false;

	*fstype = gtype->default_fs_type;
#ifdef DEFAULT_UFS2
	if (gtype->default_fs_type == FS_BSDFFS)
		*fs_sub_type = 2;
	else
#endif
		*fs_sub_type = 0;
	return true;
}

static const struct part_type_desc *
gpt_get_uuid_part_type(const uuid_t *id)
{
	char str[GUID_STR_LEN], desc[GUID_STR_LEN + MENUSTRSIZE];
	const struct gpt_ptype_desc *t;
	char *guid = NULL;
	uint32_t err;

	uuid_to_string(id, &guid, &err);
	strlcpy(str, err == uuid_s_ok ? guid : "-", sizeof str);
	free(guid);

	t = gpt_find_guid_type(str);
	if (t == NULL) {
		snprintf(desc, sizeof desc, "%s (%s)",
		    msg_string(MSG_custom_type), str);
		gpt_internal_add_ptype(str, str, desc);
		t = gpt_find_guid_type(str);
		assert(t != NULL);
	}
	return &t->gent;
}

static const struct part_type_desc *
gpt_create_custom_part_type(const char *custom, const char **err_msg)
{
	uuid_t id;
	uint32_t err;

	uuid_from_string(custom, &id, &err);
	if (err_msg != NULL &&
	   (err == uuid_s_invalid_string_uuid || err == uuid_s_bad_version)) {
		*err_msg = MSG_invalid_guid;
		return NULL;
	}
	if (err != uuid_s_ok)
		return NULL;

	return gpt_get_uuid_part_type(&id);
}

static const struct part_type_desc *
gpt_create_unknown_part_type(void)
{
	uuid_t id;
	uint32_t err;

	uuid_create(&id, &err);
	if (err != uuid_s_ok)
		return NULL;

	return gpt_get_uuid_part_type(&id);
}

static daddr_t
gpt_get_part_alignment(const struct disk_partitions *parts)
{

	assert(parts->disk_size > 0);
	if (parts->disk_size < 0)
		return 1;

	/* Use 1MB offset/alignemnt for large (>128GB) disks */
	if (parts->disk_size > HUGE_DISK_SIZE)
		return 2048;
	else if (parts->disk_size > TINY_DISK_SIZE)
		return 64;
	else
		return 4;
}

static bool
gpt_can_add_partition(const struct disk_partitions *arg)
{
	const struct gpt_disk_partitions *parts =
	    (const struct gpt_disk_partitions*)arg;
	struct disk_part_free_space space;
	daddr_t align;

	if (parts->dp.num_part >= parts->max_num_parts)
		return false;

	align = gpt_get_part_alignment(arg);
	if (parts->dp.free_space <= align)
		return false;

	if (gpt_get_free_spaces_internal(parts, &space, 1, align, align,
	    0, -1) < 1)
		return false;

	return true;
}

static bool
gpt_info_to_part(struct gpt_part_entry *p, const struct disk_part_info *info,
    const char **err_msg)
{
	p->gp_type = gpt_find_native_type(info->nat_type);
	p->gp_start = info->start;
	p->gp_size = info->size;
	if (info->last_mounted != NULL && info->last_mounted !=
	    p->last_mounted) {
		free(__UNCONST(p->last_mounted));
		p->last_mounted = strdup(info->last_mounted);
	}
	p->fs_type = info->fs_type;
	p->fs_sub_type = info->fs_sub_type;
	p->fs_opt1 = info->fs_opt1;
	p->fs_opt2 = info->fs_opt2;
	p->fs_opt3 = info->fs_opt3;

	return true;
}

static part_id
gpt_add_part(struct disk_partitions *arg,
    const struct disk_part_info *info, const char **err_msg)
{
	struct gpt_disk_partitions *parts =
	    (struct gpt_disk_partitions*)arg;
	struct disk_part_free_space space;
	struct disk_part_info data = *info;
	struct gpt_part_entry *p;
	bool ok;

	if (err_msg != NULL)
		*err_msg = NULL;

	if (gpt_get_free_spaces_internal(parts, &space, 1, 1, 1,
	    info->start, -1) < 1) {
		if (err_msg)
			*err_msg = msg_string(MSG_No_free_space);
		return NO_PART;
	}
	if (parts->dp.num_part >= parts->max_num_parts) {
		if (err_msg)
			*err_msg = msg_string(MSG_err_too_many_partitions);
		return NO_PART;
	}

	if (data.size > space.size)
		data.size = space.size;

	p = calloc(1, sizeof(*p));
	if (p == NULL) {
		if (err_msg != NULL)
			*err_msg = INTERNAL_ERROR;
		return NO_PART;
	}
	if (!gpt_info_to_part(p, &data, err_msg)) {
		free(p);
		return NO_PART;
	}
	p->gp_flags |= GPEF_MODIFIED;
	ok = gpt_insert_part_into_list(parts, &parts->partitions, p, err_msg);
	if (ok) {
		parts->dp.num_part++;
		parts->dp.free_space -= p->gp_size;
		return parts->dp.num_part-1;
	} else {
		free(p);
		return NO_PART;
	}
}

static bool
gpt_delete_partition(struct disk_partitions *arg, part_id id,
    const char **err_msg)
{
	struct gpt_disk_partitions *parts = (struct gpt_disk_partitions*)arg;
	struct gpt_part_entry *p, *last = NULL;
	part_id i;
	bool res;

	if (parts->dp.num_part == 0)
		return false;

	for (i = 0, p = parts->partitions;
	    i != id && i < parts->dp.num_part && p != NULL;
	    i++, p = p->gp_next)
		last = p;

	if (p == NULL) {
		if (err_msg)
			*err_msg = INTERNAL_ERROR;
		return false;
	}

	if (last == NULL)
		parts->partitions = p->gp_next;
	else
		last->gp_next = p->gp_next;

	res = true;
	if (p->gp_flags & GPEF_ON_DISK) {
		if (!gpt_insert_part_into_list(parts, &parts->obsolete,
		    p, err_msg))
			res = false;
	} else {
		free(p);
	}

	if (res) {
		parts->dp.num_part--;
		parts->dp.free_space += p->gp_size;
	}

	return res;
}

static bool
gpt_delete_all_partitions(struct disk_partitions *arg)
{
	struct gpt_disk_partitions *parts = (struct gpt_disk_partitions*)arg;

	while (parts->dp.num_part > 0) {
		if (!gpt_delete_partition(&parts->dp, 0, NULL))
			return false;
	}

	return true;
}

static bool
gpt_read_part(const char *disk, daddr_t start, struct gpt_part_entry *p)
{
	char *textbuf, *t, *tt;
	static const char expected_hdr[] = "Details for index ";

	/* run gpt show for this partition */
	if (collect(T_OUTPUT, &textbuf,
	    "gpt -r show -b %" PRIu64 " %s 2>/dev/null", start, disk) < 1)
		return false;

	/*
	 * gpt show should respond with single partition details, but will
	 * fall back to "show -a" output if something is wrong
	 */
	t = strtok(textbuf, "\n"); /* first line is special */
	if (strncmp(t, expected_hdr, sizeof(expected_hdr)-1) != 0) {
		free(textbuf);
		return false;
	}

	/* parse output into "old" */
	while ((t = strtok(NULL, "\n")) != NULL) {
		tt = strsep(&t, " \t");
		if (strlen(tt) == 0)
			continue;
		gpt_add_info(p, tt, t, true);
	}
	free(textbuf);

	return true;
}

static bool
gpt_apply_attr(const char *disk, const char *cmd, off_t start, uint todo)
{
	size_t i;
	char attr_str[STRSIZE];

	if (todo == 0)
		return true;

	strcpy(attr_str, "-a ");
	for (i = 0; todo != 0; i++) {
		if (!(gpt_avail_attrs[i].flag & todo))
			continue;
		todo &= ~gpt_avail_attrs[i].flag;
		if (attr_str[0])
			strlcat(attr_str, ",",
			    sizeof(attr_str));
		strlcat(attr_str,
		    gpt_avail_attrs[i].name,
		    sizeof(attr_str));
	}
	if (run_program(RUN_SILENT,
	    "gpt %s %s -b %" PRIu64 " %s", cmd, attr_str, start, disk) != 0)
		return false;
	return true;
}

/*
 * Modify an existing on-disk partition.
 * Start and size can not be changed here, caller needs to deal
 * with that kind of changes upfront.
 */
static bool
gpt_modify_part(const char *disk, struct gpt_part_entry *p)
{
	struct gpt_part_entry old;
	uint todo_set, todo_unset;

	/*
	 * Query current on-disk state
	 */
	memset(&old, 0, sizeof old);
	if (!gpt_read_part(disk, p->gp_start, &old))
		return false;

	/* Reject unsupported changes */
	if (old.gp_start != p->gp_start || old.gp_size != p->gp_size)
		return false;

	/*
	 * GUID should never change, but the internal copy
	 * may not yet know it.
	 */
	strcpy(p->gp_id, old.gp_id);

	/* Check type */
	if (p->gp_type != old.gp_type) {
		if (run_program(RUN_SILENT,
		    "gpt type -b %" PRIu64 " -T %s %s",
		    p->gp_start, p->gp_type->tid, disk) != 0)
			return false;
	}

	/* Check label */
	if (strcmp(p->gp_label, old.gp_label) != 0) {
		if (run_program(RUN_SILENT,
		    "gpt label -b %" PRIu64 " -l \'%s\' %s",
		    p->gp_start, p->gp_label, disk) != 0)
			return false;
	}

	/* Check attributes */
	if (p->gp_attr != old.gp_attr) {
		if (p->gp_attr == 0) {
			if (run_program(RUN_SILENT,
			    "gpt set -N -b %" PRIu64 " %s",
			    p->gp_start, disk) != 0)
				return false;
		} else {
			todo_set = (p->gp_attr ^ old.gp_attr) & p->gp_attr;
			todo_unset = (p->gp_attr ^ old.gp_attr) & old.gp_attr;
			if (!gpt_apply_attr(disk, "unset", p->gp_start,
			    todo_unset))
				return false;
			if (!gpt_apply_attr(disk, "set", p->gp_start,
			    todo_set))
				return false;
		}
	}

	return true;
}

/*
 * verbatim copy from sys/dev/dkwedge/dkwedge_bsdlabel.c:
 *  map FS_* to wedge strings
 */
static const char *
bsdlabel_fstype_to_str(uint8_t fstype)
{
	const char *str;

	/*
	 * For each type known to FSTYPE_DEFN (from <sys/disklabel.h>),
	 * a suitable case branch will convert the type number to a string.
	 */
	switch (fstype) {
#define FSTYPE_TO_STR_CASE(tag, number, name, fsck, mount) \
	case __CONCAT(FS_,tag):	str = __CONCAT(DKW_PTYPE_,tag);			break;
	FSTYPE_DEFN(FSTYPE_TO_STR_CASE)
#undef FSTYPE_TO_STR_CASE
	default:		str = NULL;			break;
	}

	return (str);
}

/*
 * diskfd is an open file descriptor for a disk we had trouble with
 * creating some new wedges.
 * Go through all wedges actually on that disk, check if we have a
 * record for them and remove all others.
 * This should sync our internal model of partitions with the real state.
 */
static void
gpt_sanitize(int diskfd, const struct gpt_disk_partitions *parts,
    struct gpt_part_entry *ignore)
{
	struct dkwedge_info *dkw, delw;
	struct dkwedge_list dkwl;
	size_t bufsize;
	u_int i;

	dkw = NULL;
	dkwl.dkwl_buf = dkw;
	dkwl.dkwl_bufsize = 0;

	/* get a list of all wedges */
	for (;;) {
		if (ioctl(diskfd, DIOCLWEDGES, &dkwl) == -1)
			return;
		if (dkwl.dkwl_nwedges == dkwl.dkwl_ncopied)
			break;
		bufsize = dkwl.dkwl_nwedges * sizeof(*dkw);
		if (dkwl.dkwl_bufsize < bufsize) {
			dkw = realloc(dkwl.dkwl_buf, bufsize);
			if (dkw == NULL)
				return;
			dkwl.dkwl_buf = dkw;
			dkwl.dkwl_bufsize = bufsize;
		}
	}

	/* try to remove all the ones we do not know about */
	for (i = 0; i < dkwl.dkwl_nwedges; i++) {
		bool found = false;
		const char *devname = dkw[i].dkw_devname;

		for (struct gpt_part_entry *pe = parts->partitions;
		    pe != NULL; pe = pe->gp_next) {
			if (pe == ignore)
				continue;
			if ((pe->gp_flags & GPEF_WEDGE) &&
			    strcmp(pe->gp_dev_name, devname) == 0) {
				found = true;
				break;
			}
		}
		if (found)
			continue;
		memset(&delw, 0, sizeof(delw));
		strlcpy(delw.dkw_devname, devname, sizeof(delw.dkw_devname));
		(void)ioctl(diskfd, DIOCDWEDGE, &delw);
	}

	/* cleanup */
	free(dkw);
}

static bool
gpt_add_wedge(const char *disk, struct gpt_part_entry *p,
    const struct gpt_disk_partitions *parts)
{
	struct dkwedge_info dkw;
	const char *tname;
	char diskpath[MAXPATHLEN];
	int fd;

	memset(&dkw, 0, sizeof(dkw));
	tname = bsdlabel_fstype_to_str(p->fs_type);
	if (tname)
		strlcpy(dkw.dkw_ptype, tname, sizeof(dkw.dkw_ptype));

	strlcpy((char*)&dkw.dkw_wname, p->gp_id, sizeof(dkw.dkw_wname));
	dkw.dkw_offset = p->gp_start;
	dkw.dkw_size = p->gp_size;
	if (dkw.dkw_wname[0] == 0) {
		if (p->gp_label[0] != 0)
				strlcpy((char*)&dkw.dkw_wname,
				    p->gp_label, sizeof(dkw.dkw_wname));
	}
	if (dkw.dkw_wname[0] == 0) {
		snprintf((char*)dkw.dkw_wname, sizeof dkw.dkw_wname,
		    "%s_%" PRIi64 "@%" PRIi64, disk, p->gp_size, p->gp_start);
	}

	fd = opendisk(disk, O_RDWR, diskpath, sizeof(diskpath), 0);
	if (fd < 0)
		return false;
	if (ioctl(fd, DIOCAWEDGE, &dkw) == -1) {
		if (errno == EINVAL) {
			/* sanitize existing wedges and try again */
			gpt_sanitize(fd, parts, p);
			if (ioctl(fd, DIOCAWEDGE, &dkw) == 0)
				goto ok;
		}
		close(fd);
		return false;
	}
ok:
	close(fd);

	strlcpy(p->gp_dev_name, dkw.dkw_devname, sizeof(p->gp_dev_name));
	p->gp_flags |= GPEF_WEDGE;
	return true;
}

static void
escape_spaces(char *dest, const char *src)
{
	unsigned char c;

	while (*src) {
		c = *src++;
		if (isspace(c) || c == '\\')
			*dest++ = '\\';
		*dest++ = c;
	}
	*dest = 0;
}

static bool
gpt_get_part_device(const struct disk_partitions *arg,
    part_id id, char *devname, size_t max_devname_len, int *part,
    enum dev_name_usage usage, bool with_path, bool life)
{
	const struct gpt_disk_partitions *parts =
	    (const struct gpt_disk_partitions*)arg;
	struct  gpt_part_entry *p = parts->partitions;
	char tmpname[GPT_LABEL_LEN*2];
	part_id no;


	for (no = 0; p != NULL && no < id; no++)
		p = p->gp_next;

	if (no != id || p == NULL)
		return false;

	if (part)
		*part = -1;

	if (usage == logical_name && p->gp_label[0] == 0 && p->gp_id[0] == 0)
		usage = plain_name;
	if (usage == plain_name || usage == raw_dev_name)
		life = true;
	if (!(p->gp_flags & GPEF_WEDGE) && life &&
	    !gpt_add_wedge(arg->disk, p, parts))
		return false;

	switch (usage) {
	case logical_name:
		if (p->gp_label[0] != 0) {
			escape_spaces(tmpname, p->gp_label);
			snprintf(devname, max_devname_len,
			    "NAME=%s", tmpname);
		} else {
			snprintf(devname, max_devname_len,
			    "NAME=%s", p->gp_id);
		}
		break;
	case plain_name:
		assert(p->gp_flags & GPEF_WEDGE);
		if (with_path)
			snprintf(devname, max_devname_len, _PATH_DEV "%s",
			    p->gp_dev_name);
		else
			strlcpy(devname, p->gp_dev_name, max_devname_len);
		break;
	case raw_dev_name:
		assert(p->gp_flags & GPEF_WEDGE);
		if (with_path)
			snprintf(devname, max_devname_len, _PATH_DEV "r%s",
			    p->gp_dev_name);
		else
			snprintf(devname, max_devname_len, "r%s",
			    p->gp_dev_name);
		break;
	default:
		return false;
	}

	return true;
}

static bool
gpt_write_to_disk(struct disk_partitions *arg)
{
	struct gpt_disk_partitions *parts = (struct gpt_disk_partitions*)arg;
	struct gpt_part_entry *p, *n;
	char label_arg[sizeof(p->gp_label) + 10];
	char diskpath[MAXPATHLEN];
	int fd, bits = 0;
	bool root_is_new = false, efi_is_new = false;
	part_id root_id = NO_PART, efi_id = NO_PART, pno;

	/*
	 * Remove all wedges on this disk - they may become invalid and we
	 * have no easy way to associate them with the partitioning data.
	 * Instead we will explicitly request creation of wedges on demand
	 * later.
	 */
	fd = opendisk(arg->disk, O_RDWR, diskpath, sizeof(diskpath), 0);
	if (fd < 0)
		return false;
	if (ioctl(fd, DIOCRMWEDGES, &bits) == -1)
		return false;
	close(fd);

	/*
	 * Collect first root and efi partition (if available), clear
	 * "have wedge" flags.
	 */
	for (pno = 0, p = parts->partitions; p != NULL; p = p->gp_next, pno++) {
		p->gp_flags &= ~GPEF_WEDGE;
		if (root_id == NO_PART && p->gp_type != NULL) {
			if (p->gp_type->gent.generic_ptype == PT_root &&
			    (p->gp_flags & GPEF_TARGET)) {
				root_id = pno;
				root_is_new = !(p->gp_flags & GPEF_ON_DISK);
			} else if (efi_id == NO_PART &&
			    p->gp_type->gent.generic_ptype == PT_EFI_SYSTEM) {
				efi_id = pno;
				efi_is_new = !(p->gp_flags & GPEF_ON_DISK);
			}
		}
	}

	/*
	 * If no GPT on disk yet, create it.
	 */
	if (!parts->has_gpt) {
		char limit[30];

		if (parts->max_num_parts > 0)
			sprintf(limit, "-p %zu", parts->max_num_parts);
		else
			limit[0] = 0;
		if (run_program(RUN_SILENT, "gpt create %s %s",
		    limit, parts->dp.disk))
			return false;
		parts->has_gpt = true;
	}

	/*
	 * Delete all old partitions
	 */
	for (p = parts->obsolete; p != NULL; p = n) {
		run_program(RUN_SILENT, "gpt -n remove -b %" PRIu64 " %s",
		    p->gp_start, arg->disk);
		n = p->gp_next;
		free(p);
	}
	parts->obsolete = NULL;

	/*
	 * Modify existing but changed partitions
	 */
	for (p = parts->partitions; p != NULL; p = p->gp_next) {
		if (!(p->gp_flags & GPEF_ON_DISK))
			continue;

		if (p->gp_flags & GPEF_RESIZED) {
			run_program(RUN_SILENT,
			    "gpt -n resize -b %" PRIu64 " -s %" PRIu64 "s %s",
			    p->gp_start, p->gp_size, arg->disk);
			p->gp_flags &= ~GPEF_RESIZED;
		}

		if (!(p->gp_flags & GPEF_MODIFIED))
			continue;

		if (!gpt_modify_part(parts->dp.disk, p))
			return false;
	}

	/*
	 * Add new partitions
	 */
	for (p = parts->partitions; p != NULL; p = p->gp_next) {
		if (p->gp_flags & GPEF_ON_DISK)
			continue;
		if (!(p->gp_flags & GPEF_MODIFIED))
			continue;

		if (p->gp_label[0] == 0)
			label_arg[0] = 0;
		else
			sprintf(label_arg, "-l \'%s\'", p->gp_label);

		if (p->gp_type != NULL)
			run_program(RUN_SILENT,
			    "gpt -n add -b %" PRIu64 " -s %" PRIu64
			    "s -t %s %s %s",
			    p->gp_start, p->gp_size, p->gp_type->tid,
			    label_arg, arg->disk);
		else
			run_program(RUN_SILENT,
			    "gpt -n add -b %" PRIu64 " -s %" PRIu64
			    "s %s %s",
			    p->gp_start, p->gp_size, label_arg, arg->disk);
		gpt_apply_attr(arg->disk, "set", p->gp_start, p->gp_attr);
		gpt_read_part(arg->disk, p->gp_start, p);
		p->gp_flags |= GPEF_ON_DISK;
	}

	/*
	 * Additional MD bootloader magic...
	 */
	if (!md_gpt_post_write(&parts->dp, root_id, root_is_new, efi_id,
	    efi_is_new))
		return false;

	return true;
}

static part_id
gpt_find_by_name(struct disk_partitions *arg, const char *name)
{
	struct gpt_disk_partitions *parts = (struct gpt_disk_partitions*)arg;
	struct gpt_part_entry *p;
	part_id pno;

	for (pno = 0, p = parts->partitions; p != NULL;
	    p = p->gp_next, pno++) {
		if (strcmp(p->gp_label, name) == 0)
			return pno;
		if (strcmp(p->gp_id, name) == 0)
			return pno;
	}

	return NO_PART;
}

bool
gpt_parts_check(void)
{

	check_available_binaries();

	return have_gpt && have_dk;
}

static void
gpt_free(struct disk_partitions *arg)
{
	struct gpt_disk_partitions *parts = (struct gpt_disk_partitions*)arg;
	struct gpt_part_entry *p, *n;

	assert(parts != NULL);
	for (p = parts->partitions; p != NULL; p = n) {
		if (p->gp_flags & GPEF_WEDGE)
			register_post_umount_delwedge(parts->dp.disk,
			    p->gp_dev_name);
		free(__UNCONST(p->last_mounted));
		n = p->gp_next;
		free(p);
	}
	free(__UNCONST(parts->dp.disk));
	free(parts);
}

static void
gpt_destroy_part_scheme(struct disk_partitions *arg)
{

	run_program(RUN_SILENT, "gpt destroy %s", arg->disk);
	gpt_free(arg);
}

static bool
gpt_custom_attribute_writable(const struct disk_partitions *arg,
    part_id ptn, size_t attr_no)
{
	const struct gpt_disk_partitions *parts =
	    (const struct gpt_disk_partitions*)arg;
	size_t i;
	struct gpt_part_entry *p;

	if (attr_no >= arg->pscheme->custom_attribute_count)
		return false;

	const msg label = arg->pscheme->custom_attributes[attr_no].label;

	/* we can not edit the uuid attribute */
	if (label == MSG_ptn_uuid)
		return false;

	/* the label is always editable */
	if (label == MSG_ptn_label)
		return true;

	/* the GPT type is read only */
	if (label == MSG_ptn_gpt_type)
		return false;

	/* BOOTME makes no sense on swap partitions */
	for (i = 0, p = parts->partitions; p != NULL; i++, p = p->gp_next)
		if (i == ptn)
			break;

	if (p == NULL)
		return false;

	if (p->fs_type == FS_SWAP ||
	    (p->gp_type != NULL && p->gp_type->gent.generic_ptype == PT_swap))
		return false;

	return true;
}

static const char *
gpt_get_label_str(const struct disk_partitions *arg, part_id ptn)
{
	const struct gpt_disk_partitions *parts =
	    (const struct gpt_disk_partitions*)arg;
	size_t i;
	struct gpt_part_entry *p;

	for (i = 0, p = parts->partitions; p != NULL; i++, p = p->gp_next)
		if (i == ptn)
			break;

	if (p == NULL)
		return NULL;

	if (p->gp_label[0] != 0)
		return p->gp_label;
	return p->gp_id;
}

static bool
gpt_format_custom_attribute(const struct disk_partitions *arg,
    part_id ptn, size_t attr_no, const struct disk_part_info *info,
    char *out, size_t out_space)
{
	const struct gpt_disk_partitions *parts =
	    (const struct gpt_disk_partitions*)arg;
	size_t i;
	struct gpt_part_entry *p, data;

	for (i = 0, p = parts->partitions; p != NULL; i++, p = p->gp_next)
		if (i == ptn)
			break;

	if (p == NULL)
		return false;

	if (attr_no >= parts->dp.pscheme->custom_attribute_count)
		return false;

	const msg label = parts->dp.pscheme->custom_attributes[attr_no].label;

	if (info != NULL) {
		data = *p;
		gpt_info_to_part(&data, info, NULL);
		p = &data;
	}

	if (label == MSG_ptn_label)
		strlcpy(out, p->gp_label, out_space);
	else if (label == MSG_ptn_uuid)
		strlcpy(out, p->gp_id, out_space);
	else if (label == MSG_ptn_gpt_type) {
		if (p->gp_type != NULL)
			strlcpy(out, p->gp_type->gent.description, out_space);
		else if (out_space > 1)
			out[0] = 0;
	} else if (label == MSG_ptn_boot)
		strlcpy(out, msg_string(p->gp_attr & GPT_ATTR_BOOT ?
		    MSG_Yes : MSG_No), out_space);
	else
		return false;

	return true;
}

static bool
gpt_custom_attribute_toggle(struct disk_partitions *arg,
    part_id ptn, size_t attr_no)
{
	const struct gpt_disk_partitions *parts =
	    (const struct gpt_disk_partitions*)arg;
	size_t i;
	struct gpt_part_entry *p;

	for (i = 0, p = parts->partitions; p != NULL; i++, p = p->gp_next)
		if (i == ptn)
			break;

	if (p == NULL)
		return false;

	if (attr_no >= parts->dp.pscheme->custom_attribute_count)
		return false;

	const msg label = parts->dp.pscheme->custom_attributes[attr_no].label;
	if (label != MSG_ptn_boot)
		return false;

	if (p->gp_attr & GPT_ATTR_BOOT) {
		p->gp_attr &= ~GPT_ATTR_BOOT;
	} else {
		for (i = 0, p = parts->partitions; p != NULL;
		    i++, p = p->gp_next)
			if (i == ptn)
				p->gp_attr |= GPT_ATTR_BOOT;
			else
				p->gp_attr &= ~GPT_ATTR_BOOT;
	}
	return true;
}

static bool
gpt_custom_attribute_set_str(struct disk_partitions *arg,
    part_id ptn, size_t attr_no, const char *new_val)
{
	const struct gpt_disk_partitions *parts =
	    (const struct gpt_disk_partitions*)arg;
	size_t i;
	struct gpt_part_entry *p;

	for (i = 0, p = parts->partitions; p != NULL; i++, p = p->gp_next)
		if (i == ptn)
			break;

	if (p == NULL)
		return false;

	if (attr_no >= parts->dp.pscheme->custom_attribute_count)
		return false;

	const msg label = parts->dp.pscheme->custom_attributes[attr_no].label;

	if (label != MSG_ptn_label)
		return false;

	strlcpy(p->gp_label, new_val, sizeof(p->gp_label));
	return true;
}

static bool
gpt_have_boot_support(const char *disk)
{
#ifdef	HAVE_GPT_BOOT
	return true;
#else
	return false;
#endif
}

const struct disk_part_custom_attribute gpt_custom_attrs[] = {
	{ .label = MSG_ptn_label,	.type = pet_str },
	{ .label = MSG_ptn_uuid,	.type = pet_str },
	{ .label = MSG_ptn_gpt_type,	.type = pet_str },
	{ .label = MSG_ptn_boot,	.type = pet_bool },
};

const struct disk_partitioning_scheme
gpt_parts = {
	.name = MSG_parttype_gpt,
	.short_name = MSG_parttype_gpt_short,
	.part_flag_desc = MSG_gpt_flag_desc,
	.custom_attribute_count = __arraycount(gpt_custom_attrs),
	.custom_attributes = gpt_custom_attrs,
	.get_part_types_count = gpt_type_count,
	.get_part_type = gpt_get_ptype,
	.get_generic_part_type = gpt_get_generic_type,
	.get_fs_part_type = gpt_get_fs_part_type,
	.get_default_fstype = gpt_get_default_fstype,
	.create_custom_part_type = gpt_create_custom_part_type,
	.create_unknown_part_type = gpt_create_unknown_part_type,
	.get_part_alignment = gpt_get_part_alignment,
	.read_from_disk = gpt_read_from_disk,
	.get_cylinder_size = gpt_cyl_size,
	.create_new_for_disk = gpt_create_new,
	.have_boot_support = gpt_have_boot_support,
	.find_by_name = gpt_find_by_name,
	.can_add_partition = gpt_can_add_partition,
	.custom_attribute_writable = gpt_custom_attribute_writable,
	.format_custom_attribute = gpt_format_custom_attribute,
	.custom_attribute_toggle = gpt_custom_attribute_toggle,
	.custom_attribute_set_str = gpt_custom_attribute_set_str,
	.other_partition_identifier = gpt_get_label_str,
	.get_part_device = gpt_get_part_device,
	.max_free_space_at = gpt_max_free_space_at,
	.get_free_spaces = gpt_get_free_spaces,
	.adapt_foreign_part_info = generic_adapt_foreign_part_info,
	.get_part_info = gpt_get_part_info,
	.get_part_attr_str = gpt_get_part_attr_str,
	.set_part_info = gpt_set_part_info,
	.add_partition = gpt_add_part,
	.delete_all_partitions = gpt_delete_all_partitions,
	.delete_partition = gpt_delete_partition,
	.write_to_disk = gpt_write_to_disk,
	.free = gpt_free,
	.destroy_part_scheme = gpt_destroy_part_scheme,
	.cleanup = gpt_cleanup,
};

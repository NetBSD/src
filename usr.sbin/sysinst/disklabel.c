/*	$NetBSD: disklabel.c,v 1.10.2.1 2019/08/08 05:51:43 msaitoh Exp $	*/

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
#include "md.h"
#include <assert.h>
#include <util.h>
#include <paths.h>
#include <sys/ioctl.h>
#include <sys/param.h>

const struct disk_partitioning_scheme disklabel_parts;

/*************** disklabel ******************************************/
/* a disklabel based disk_partitions interface */
struct disklabel_disk_partitions {
	struct disk_partitions dp;
	struct disklabel l;
	daddr_t ptn_alignment;
	char last_mounted[MAXPARTITIONS][MOUNTLEN];
	uint fs_sub_type[MAXPARTITIONS];
};

/*
 * Maximum number of disklabel partitions the current kernel supports
 */
size_t dl_maxpart;

/* index into this arrray is the type code */
static struct part_type_desc dl_types[__arraycount(fstypenames)-1];

struct dl_custom_ptype {
	unsigned int type;
	char short_desc[6], description[30];
	struct part_type_desc desc;
};
struct dl_custom_ptype * dl_custom_ptypes;
size_t dl_custom_ptype_count;

static uint8_t dl_part_type_from_generic(const struct part_type_desc*);

static void
disklabel_init_default_alignment(struct disklabel_disk_partitions *parts,
    uint track)
{
	if (track == 0)
		track = MEG / 512;

	if (dl_maxpart == 0)
		dl_maxpart = getmaxpartitions();

#ifdef MD_DISKLABEL_SET_ALIGN_PRE
	if (MD_DISKLABEL_SET_ALIGN_PRE(parts->ptn_alignment, track))
		return;
#endif
	/* Use 1MB alignemnt for large (>128GB) disks */
	if (parts->dp.disk_size > HUGE_DISK_SIZE) {
		parts->ptn_alignment = 2048;
	} else if (parts->dp.disk_size > TINY_DISK_SIZE) {
		parts->ptn_alignment = 64;
	} else {
		parts->ptn_alignment = 1;
	}
#ifdef MD_DISKLABEL_SET_ALIGN_POST
	MD_DISKLABEL_SET_ALIGN_POST(parts->ptn_alignment, track);
#endif
}

static bool
disklabel_change_geom(struct disk_partitions *arg, int ncyl, int nhead,
    int nsec)
{
	struct disklabel_disk_partitions *parts =
	    (struct disklabel_disk_partitions*)arg;

	assert(parts->l.d_secsize != 0);
	assert(parts->l.d_nsectors != 0);
	assert(parts->l.d_ntracks != 0);
	assert(parts->l.d_ncylinders != 0);
	assert(parts->l.d_secpercyl != 0);

	disklabel_init_default_alignment(parts, nhead * nsec);
	if (ncyl*nhead*nsec <= TINY_DISK_SIZE)
		set_default_sizemult(1);
	else
		set_default_sizemult(MEG/512);

	return true;
}

static struct disk_partitions *
disklabel_parts_new(const char *dev, daddr_t start, daddr_t len,
    daddr_t total_size, bool is_boot_drive)
{
	struct disklabel_disk_partitions *parts;
	struct disk_geom geo;

	if (!get_disk_geom(dev, &geo))
		return NULL;

	parts = calloc(1, sizeof(*parts));
	if (parts == NULL)
		return NULL;

	if (len > disklabel_parts.size_limit)
		len = disklabel_parts.size_limit;
	if (total_size > disklabel_parts.size_limit)
		total_size = disklabel_parts.size_limit;

	parts->l.d_ncylinders = geo.dg_ncylinders;
	parts->l.d_ntracks = geo.dg_ntracks;
	parts->l.d_nsectors = geo.dg_nsectors;
	parts->l.d_secsize = geo.dg_secsize;
	parts->l.d_secpercyl = geo.dg_nsectors * geo.dg_ntracks;

	parts->dp.pscheme = &disklabel_parts;
	parts->dp.disk = dev;
	parts->dp.disk_start = start;
	parts->dp.disk_size = parts->dp.free_space = len;
	disklabel_init_default_alignment(parts, parts->l.d_secpercyl);

	strncpy(parts->l.d_packname, "fictious", sizeof parts->l.d_packname);

#if RAW_PART > 2
	parts->l.d_partitions[RAW_PART-1].p_fstype = FS_UNUSED;
	parts->l.d_partitions[RAW_PART-1].p_offset = start;
	parts->l.d_partitions[RAW_PART-1].p_size = len;
	parts->dp.num_part++;
#endif
	parts->l.d_partitions[RAW_PART].p_fstype = FS_UNUSED;
	parts->l.d_partitions[RAW_PART].p_offset = 0;
	parts->l.d_partitions[RAW_PART].p_size = total_size;
	parts->dp.num_part++;

	parts->l.d_npartitions = RAW_PART+1;

	return &parts->dp;
}

static struct disk_partitions *
disklabel_parts_read(const char *disk, daddr_t start, daddr_t len)
{
	int fd;
	char diskpath[MAXPATHLEN];
	uint flags;

	if (run_program(RUN_SILENT | RUN_ERROR_OK,
	    "disklabel -r %s", disk) != 0)
		return NULL;

	/* read partitions */

	struct disklabel_disk_partitions *parts = calloc(1, sizeof(*parts));
	if (parts == NULL)
		return NULL;

	fd = opendisk(disk, O_RDONLY, diskpath, sizeof(diskpath), 0);
	if (fd == -1) {
		free(parts);
		return NULL;
	}

	/*
	 * We should actually try to read the label inside the start/len
	 * boundary, but for simplicity just rely on the kernel and
	 * instead verify a FS_UNUSED partition at RAW_PART-1 (if
	 * RAW_PART > 'c') is within the given limits.
	 */
	if (ioctl(fd, DIOCGDINFO, &parts->l) < 0) {
		free(parts);
		close(fd);
		return NULL;
	}
#if RAW_PART > 2
	if (parts->l.d_partitions[RAW_PART-1].p_fstype == FS_UNUSED) {
		daddr_t dlstart = parts->l.d_partitions[RAW_PART-1].p_offset;
		daddr_t dlend = start +
		    parts->l.d_partitions[RAW_PART-1].p_size;

		if (dlstart < start && dlend > (start+len)) {
			assert(false);
			free(parts);
			close(fd);
			return NULL;
		}
	}
#endif

	if (len > disklabel_parts.size_limit)
		len = disklabel_parts.size_limit;
	parts->dp.pscheme = &disklabel_parts;
	parts->dp.disk = disk;
	parts->dp.disk_start = start;
	parts->dp.disk_size = parts->dp.free_space = len;
	disklabel_init_default_alignment(parts, 0);

	for (int part = 0; part < parts->l.d_npartitions; part++) {
		if (parts->l.d_partitions[part].p_fstype == FS_UNUSED
		    && parts->l.d_partitions[part].p_size == 0)
			continue;

		parts->dp.num_part++;
		if (parts->l.d_partitions[part].p_fstype == FS_UNUSED)
			continue;

		flags = 0;
		if (parts->l.d_partitions[part].p_fstype == FS_MSDOS)
			flags = GLM_MAYBE_FAT32;
		else if (parts->l.d_partitions[part].p_fstype == FS_BSDFFS)
			flags = GLM_LIKELY_FFS;
		if (flags != 0) {
			uint fs_type, fs_sub_type;
			const char *lm = get_last_mounted(fd,
			    parts->l.d_partitions[part].p_offset,
			    &fs_type, &fs_sub_type, flags);
			if (lm != NULL && *lm != 0) {
				strlcpy(parts->last_mounted[part], lm,
				    sizeof(parts->last_mounted[part]));
				if (parts->l.d_partitions[part].p_fstype ==
				    fs_type)
					parts->fs_sub_type[part] = fs_sub_type;
				canonicalize_last_mounted(
				    parts->last_mounted[part]);
			}
		}

		if (parts->l.d_partitions[part].p_size > parts->dp.free_space)
			parts->dp.free_space = 0;
		else
			parts->dp.free_space -=
			    parts->l.d_partitions[part].p_size;
	}
	close(fd);

	return &parts->dp;
}

static bool
disklabel_write_to_disk(struct disk_partitions *arg)
{
	struct disklabel_disk_partitions *parts =
	    (struct disklabel_disk_partitions*)arg;
	FILE *f;
	char fname[PATH_MAX], packname[sizeof(parts->l.d_packname)+1];
	int i, rv = 0;
	const char *disk = parts->dp.disk, *s;
	const struct partition *lp;
	char *d;
	size_t n;

	assert(parts->l.d_secsize != 0);
	assert(parts->l.d_nsectors != 0);
	assert(parts->l.d_ntracks != 0);
	assert(parts->l.d_ncylinders != 0);
	assert(parts->l.d_secpercyl != 0);

	sprintf(fname, "/tmp/disklabel.%u", getpid());
	f = fopen(fname, "w");
	if (f == NULL)
		return false;

	/* make sure we have a 0 terminated packname */
	strlcpy(packname, parts->l.d_packname, sizeof packname);

	/* fill typename with disk name prefix, if not already set */
	if (strlen(parts->l.d_typename) == 0) {
		for (n = 0, d = parts->l.d_typename, s = disk;
		    *s && n < sizeof(parts->l.d_typename); d++, s++, n++) {
			if (isdigit((unsigned char)*s))
				break;
			*d = *s;
		}
	}
	parts->l.d_typename[sizeof(parts->l.d_typename)-1] = 0;

	/* we need a valid disk type name, so enforce an arbitrary if
	 * above did not yield a usable one */
	if (strlen(parts->l.d_typename) == 0)
		strncpy(parts->l.d_typename, "SCSI",
		    sizeof(parts->l.d_typename));

	lp = parts->l.d_partitions;
	scripting_fprintf(NULL, "cat <<EOF >%s\n", fname);
	scripting_fprintf(f, "%s|NetBSD installation generated:\\\n",
	    parts->l.d_typename);
	scripting_fprintf(f, "\t:nc#%d:nt#%d:ns#%d:\\\n",
	    parts->l.d_ncylinders, parts->l.d_ntracks, parts->l.d_nsectors);
	scripting_fprintf(f, "\t:sc#%d:su#%" PRIu32 ":\\\n",
	    parts->l.d_secpercyl, lp[RAW_PART].p_offset+lp[RAW_PART].p_size);
	scripting_fprintf(f, "\t:se#%d:\\\n", parts->l.d_secsize);

	for (i = 0; i < parts->l.d_npartitions; i++) {
		scripting_fprintf(f, "\t:p%c#%" PRIu32 ":o%c#%" PRIu32
		    ":t%c=%s:", 'a'+i, (uint32_t)lp[i].p_size,
		    'a'+i, (uint32_t)lp[i].p_offset, 'a'+i,
		    getfslabelname(lp[i].p_fstype, 0));
		if (lp[i].p_fstype == FS_BSDLFS ||
		    lp[i].p_fstype == FS_BSDFFS)
			scripting_fprintf (f, "b%c#%" PRIu32 ":f%c#%" PRIu32
			    ":", 'a'+i,
			    (uint32_t)(lp[i].p_fsize *
			    lp[i].p_frag),
			    'a'+i, (uint32_t)lp[i].p_fsize);
	
		if (i < parts->l.d_npartitions - 1)
			scripting_fprintf(f, "\\\n");
		else
			scripting_fprintf(f, "\n");
	}
	scripting_fprintf(NULL, "EOF\n");

	fclose(f);

	/*
	 * Label a disk using an MD-specific string DISKLABEL_CMD for
	 * to invoke disklabel.
	 * if MD code does not define DISKLABEL_CMD, this is a no-op.
	 *
	 * i386 port uses "/sbin/disklabel -w -r", just like i386
	 * miniroot scripts, though this may leave a bogus incore label.
	 *
	 * Sun ports should use DISKLABEL_CMD "/sbin/disklabel -w"
	 * to get incore to ondisk inode translation for the Sun proms.
	 */
#ifdef DISKLABEL_CMD
	/* disklabel the disk */
	rv = run_program(RUN_DISPLAY, "%s -f %s %s %s %s",
	    DISKLABEL_CMD, fname, disk, parts->l.d_typename, packname);
#endif

	unlink(fname);

	return rv == 0;
}

static bool
disklabel_delete_all(struct disk_partitions *arg)
{
	struct disklabel_disk_partitions *parts =
	    (struct disklabel_disk_partitions*)arg;
	daddr_t total_size = parts->l.d_partitions[RAW_PART].p_size;

	memset(&parts->l.d_partitions, 0, sizeof(parts->l.d_partitions));
	parts->dp.num_part = 0;

#if RAW_PART > 2
	parts->l.d_partitions[RAW_PART-1].p_fstype = FS_UNUSED;
	parts->l.d_partitions[RAW_PART-1].p_offset = parts->dp.disk_start;
	parts->l.d_partitions[RAW_PART-1].p_size = parts->dp.disk_size;
	parts->dp.num_part++;
#endif
	parts->l.d_partitions[RAW_PART].p_fstype = FS_UNUSED;
	parts->l.d_partitions[RAW_PART].p_offset = 0;
	parts->l.d_partitions[RAW_PART].p_size = total_size;
	parts->dp.num_part++;

	parts->l.d_npartitions = RAW_PART+1;
	return true;
}

static bool
disklabel_delete(struct disk_partitions *arg, part_id id,
    const char **err_msg)
{
	struct disklabel_disk_partitions *parts =
	    (struct disklabel_disk_partitions*)arg;
	part_id ndx;

	ndx = 0;
	for (int part = 0; part < parts->l.d_npartitions; part++) {
		if (parts->l.d_partitions[part].p_fstype == FS_UNUSED
		    && parts->l.d_partitions[part].p_size == 0)
			continue;

		if (ndx == id) {
			if (part == RAW_PART
#if RAW_PART > 2
				|| part == RAW_PART-1
#endif
						) {
				if (err_msg)
					*err_msg = msg_string(
					    MSG_part_not_deletable);
				return false;
			}
			parts->l.d_partitions[part].p_size = 0;
			parts->l.d_partitions[part].p_offset = 0;
			parts->l.d_partitions[part].p_fstype = FS_UNUSED;
			parts->dp.num_part--;
			return true;
		}
		ndx++;
	}

	if (err_msg)
		*err_msg = INTERNAL_ERROR;
	return false;
}

static bool
disklabel_delete_range(struct disk_partitions *arg, daddr_t r_start,
    daddr_t r_size)
{
	struct disklabel_disk_partitions *parts =
	    (struct disklabel_disk_partitions*)arg;

	for (int part = 0; part < parts->l.d_npartitions; part++) {
		if (parts->l.d_partitions[part].p_fstype == FS_UNUSED
		    && parts->l.d_partitions[part].p_size == 0)
			continue;

		if (part == RAW_PART)
			continue;

		daddr_t start = parts->l.d_partitions[part].p_offset;
		daddr_t end = start + parts->l.d_partitions[part].p_size;

#if RAW_PART > 2
		if (part == RAW_PART - 1 && start == r_start &&
		    r_start + r_size == end)
			continue;
#endif

		if ((start >= r_start && start <= r_start+r_size) ||
		    (end >= r_start && end <= r_start+r_size)) {
			if (parts->dp.num_part > 1)
				parts->dp.num_part--;
			parts->dp.free_space +=
			    parts->l.d_partitions[part].p_size;
			parts->l.d_partitions[part].p_fstype = FS_UNUSED;
			parts->l.d_partitions[part].p_size = 0;
		}
	}

	return true;
}

static void
dl_init_types(void)
{
	for (size_t i = 0; i < __arraycount(dl_types); i++) {
		if (fstypenames[i] == NULL)
			break;
		dl_types[i].short_desc =
		dl_types[i].description = getfslabelname(i, 0);
		enum part_type pt;
		switch (i) {
		case FS_UNUSED:	pt = PT_undef; break;
		case FS_BSDFFS:	pt = PT_root; break;
		case FS_SWAP:	pt = PT_swap; break;
		case FS_MSDOS:	pt = PT_FAT; break;
		default:	pt = PT_unknown; break;
		}
		dl_types[i].generic_ptype = pt;
	}
}

static uint8_t
dl_part_type_from_generic(const struct part_type_desc *gent)
{

	if (dl_types[0].description == NULL)
		dl_init_types();
	for (size_t i = 0; i < __arraycount(dl_types); i++)
		if (gent == &dl_types[i])
			return (uint8_t)i;

	for (size_t i = 0; i < dl_custom_ptype_count; i++)
		if (gent == &dl_custom_ptypes[i].desc)
			return dl_custom_ptypes[i].type;

	return 0;
}

static size_t
disklabel_type_count(void)
{
	return __arraycount(dl_types) + dl_custom_ptype_count;
}

static const struct part_type_desc *
disklabel_get_type(size_t ndx)
{
	if (dl_types[0].description == NULL)
		dl_init_types();

	if (ndx < __arraycount(dl_types))
		return &dl_types[ndx];

	ndx -= __arraycount(dl_types);
	if (ndx >= dl_custom_ptype_count)
		return NULL;

	return &dl_custom_ptypes[ndx].desc;
}

static const struct part_type_desc *
disklabel_find_type(uint type, bool create_if_unknown)
{
	if (dl_types[0].description == NULL)
		dl_init_types();

	if (type < __arraycount(dl_types))
		return &dl_types[type];

	for (size_t i = 0; i < dl_custom_ptype_count; i++)
		if (dl_custom_ptypes[i].type == type)
			return &dl_custom_ptypes[i].desc;

	if (create_if_unknown) {
		struct dl_custom_ptype *nt;

		nt = realloc(dl_custom_ptypes, dl_custom_ptype_count+1);
		if (nt == NULL)
			return NULL;
		dl_custom_ptypes = nt;
		nt = dl_custom_ptypes + dl_custom_ptype_count;
		dl_custom_ptype_count++;
		memset(nt, 0, sizeof(*nt));
		nt->type = type;
		snprintf(nt->short_desc, sizeof(nt->short_desc), "%u", type);
		nt->short_desc[sizeof(nt->short_desc)-1] = 0;
		snprintf(nt->description, sizeof(nt->description),
		    "%s (%u)", msg_string(MSG_custom_type), type);
		nt->description[sizeof(nt->description)-1] = 0;
		nt->desc.generic_ptype = PT_unknown;
		nt->desc.short_desc = nt->short_desc;
		nt->desc.description = nt->description;
		return &nt->desc;
	}

	return NULL;
}

static const struct part_type_desc *
disklabel_create_custom_part_type(const char *custom, const char **err_msg)
{
	char *endp;
	unsigned long fstype;

	fstype = strtoul(custom, &endp, 10);
	if (*endp != 0) {
		if (err_msg)
			*err_msg = msg_string(MSG_dl_type_invalid);
		return NULL;
	}

	return disklabel_find_type(fstype, true);
}

static const struct part_type_desc *
disklabel_get_fs_part_type(unsigned fstype, unsigned subtype)
{
	return disklabel_find_type(fstype, false);
}

static const struct part_type_desc *
disklabel_get_generic_type(enum part_type pt)
{
	size_t nt;

	if (dl_types[0].description == NULL)
		dl_init_types();

	switch (pt) {
	case PT_root:	nt = FS_BSDFFS; break;
	case PT_swap:	nt = FS_SWAP; break;
	case PT_FAT:
	case PT_EFI_SYSTEM:
			nt = FS_MSDOS; break;
	default:	nt = FS_UNUSED; break;
	}

	return disklabel_get_type(nt);
}

static bool
disklabel_get_part_info(const struct disk_partitions *arg, part_id id,
    struct disk_part_info *info)
{
	const struct disklabel_disk_partitions *parts =
	    (const struct disklabel_disk_partitions*)arg;
	part_id ndx;

	if (dl_types[0].description == NULL)
		dl_init_types();

	ndx = 0;
	for (int part = 0; part < parts->l.d_npartitions; part++) {
		if (parts->l.d_partitions[part].p_fstype == FS_UNUSED
		    && parts->l.d_partitions[part].p_size == 0)
			continue;

		if (ndx == id) {
			memset(info, 0, sizeof(*info));
			info->start = parts->l.d_partitions[part].p_offset;
			info->size = parts->l.d_partitions[part].p_size;
			info->nat_type = disklabel_find_type(
			    parts->l.d_partitions[part].p_fstype, true);
			if (parts->last_mounted[part][0] != 0)
				info->last_mounted = parts->last_mounted[part];
			info->fs_type = parts->l.d_partitions[part].p_fstype;
			info->fs_sub_type = parts->fs_sub_type[part];
			if (part == RAW_PART &&
			    parts->l.d_partitions[part].p_fstype == FS_UNUSED)
				info->flags |=
				    PTI_PSCHEME_INTERNAL|PTI_RAW_PART;
#if RAW_PART > 2
			if (part == (RAW_PART-1) &&
			    parts->l.d_partitions[part].p_fstype == FS_UNUSED)
				info->flags |=
				    PTI_PSCHEME_INTERNAL|PTI_WHOLE_DISK;
#endif
			return true;
		}

		ndx++;
		if (ndx > parts->dp.num_part || ndx > id)
			break;
	}

	return false;
}

static bool
disklabel_set_part_info(struct disk_partitions *arg, part_id id,
    const struct disk_part_info *info, const char **err_msg)
{
	struct disklabel_disk_partitions *parts =
	    (struct disklabel_disk_partitions*)arg;
	part_id ndx;

	if (dl_types[0].description == NULL)
		dl_init_types();

	ndx = 0;
	for (int part = 0; part < parts->l.d_npartitions; part++) {
		if (parts->l.d_partitions[part].p_fstype == FS_UNUSED
		    && parts->l.d_partitions[part].p_size == 0)
			continue;

		if (ndx == id) {
			parts->l.d_partitions[part].p_offset = info->start;
			parts->l.d_partitions[part].p_size = info->size;
			parts->l.d_partitions[part].p_fstype =
			    dl_part_type_from_generic(info->nat_type);
			if (info->last_mounted != NULL &&
			    info->last_mounted != parts->last_mounted[part])
				strlcpy(parts->last_mounted[part],
				    info->last_mounted,
				    sizeof(parts->last_mounted[part]));
			assert(info->fs_type == 0 || info->fs_type ==
			    parts->l.d_partitions[part].p_fstype);
			if (info->fs_sub_type != 0)
				parts->fs_sub_type[part] = info->fs_sub_type;
			return true;
		}

		ndx++;
		if (ndx > parts->dp.num_part || ndx > id)
			break;
	}

	return false;
}

static size_t
disklabel_get_free_spaces_internal(const struct
    disklabel_disk_partitions *parts,
    struct disk_part_free_space *result, size_t max_num_result,
    daddr_t min_space_size, daddr_t align, daddr_t start, daddr_t ignore)
{
	size_t cnt = 0, i;
	daddr_t s, e, from, size, end_of_disk;

	if (start < parts->dp.disk_start)
		start = parts->dp.disk_start;
	if (min_space_size < 1)
		min_space_size = 1;
	if (align > 1 && (start % align) != 0)
		start = max(roundup(start, align), align);
	end_of_disk = parts->dp.disk_start + parts->dp.disk_size;
	from = start;
	while (from < end_of_disk && cnt < max_num_result) {
again:
		size = parts->dp.disk_start + parts->dp.disk_size - from;
		start = from;
		for (i = 0; i < parts->l.d_npartitions; i++) {
			if (i == RAW_PART)
				continue;
			if (parts->l.d_partitions[i].p_fstype == FS_UNUSED)
				continue;

			s = parts->l.d_partitions[i].p_offset;
			e = parts->l.d_partitions[i].p_size + s;
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

static bool
disklabel_can_add_partition(const struct disk_partitions *arg)
{
	const struct disklabel_disk_partitions *parts =
	    (const struct disklabel_disk_partitions*)arg;
	struct disk_part_free_space space;
	int i;

	if (dl_maxpart == 0)
		dl_maxpart = getmaxpartitions();
	if (parts->dp.free_space < parts->ptn_alignment)
		return false;
	if (parts->dp.num_part >= dl_maxpart)
		return false;
	if (disklabel_get_free_spaces_internal(parts, &space, 1,
	    parts->ptn_alignment, parts->ptn_alignment, 0, -1) < 1)
		return false;

	for (i = 0; i < parts->l.d_npartitions; i++) {
		if (i == RAW_PART)
			continue;
#if RAW_PART > 2
		if (i == RAW_PART-1)
			continue;
#endif
		if (parts->l.d_partitions[i].p_fstype == FS_UNUSED)
			return true;
	}
	return false;
}

static bool
disklabel_get_disk_pack_name(const struct disk_partitions *arg,
    char *buf, size_t len)
{
	const struct disklabel_disk_partitions *parts =
	    (const struct disklabel_disk_partitions*)arg;

	strlcpy(buf, parts->l.d_packname, min(len,
	    sizeof(parts->l.d_packname)+1));
	return true;
}

static bool
disklabel_set_disk_pack_name(struct disk_partitions *arg, const char *pack)
{
	struct disklabel_disk_partitions *parts =
	    (struct disklabel_disk_partitions*)arg;

	strncpy(parts->l.d_packname, pack, sizeof(parts->l.d_packname));
	return true;
}

static bool
disklabel_get_part_device(const struct disk_partitions *arg,
    part_id ptn, char *devname, size_t max_devname_len, int *part,
    enum dev_name_usage which_name, bool with_path)
{

	if (part != 0)
		*part = ptn;

	switch (which_name) {
	case parent_device_only:
		strlcpy(devname, arg->disk, max_devname_len);
		return true;
	case logical_name:
	case plain_name:
		if (with_path)
			snprintf(devname, max_devname_len, _PATH_DEV "%s%c",
			    arg->disk, (char)ptn + 'a');
		else
			snprintf(devname, max_devname_len, "%s%c",
			    arg->disk, (char)ptn + 'a');
		return true;
	case raw_dev_name:
		if (with_path)
			snprintf(devname, max_devname_len, _PATH_DEV "r%s%c",
			    arg->disk, (char)ptn + 'a');
		else
			snprintf(devname, max_devname_len, "r%s%c",
			    arg->disk, (char)ptn + 'a');
		return true;
	}

	return false;
}

static part_id
disklabel_add_partition(struct disk_partitions *arg,
    const struct disk_part_info *info, const char **err_msg)
{
	struct disklabel_disk_partitions *parts =
	    (struct disklabel_disk_partitions*)arg;
	int i, part = -1;
	part_id new_id;
	struct disk_part_free_space space;
	struct disk_part_info data = *info;

	if (disklabel_get_free_spaces_internal(parts, &space, 1, 1, 1,
	    info->start, -1) < 1) {
		if (err_msg)
			*err_msg = msg_string(MSG_No_free_space);
		return NO_PART;
	}
	if (data.size > space.size)
		data.size = space.size;
	daddr_t dend = data.start+data.size;
	if (space.start > data.start)
		data.start = space.start;
	if (space.start + space.size < dend)
		data.size = space.start+space.size-data.start;

	if (dl_maxpart == 0)
		dl_maxpart = getmaxpartitions();

	for (new_id = 0, i = 0; i < parts->l.d_npartitions; i++) {
		if (parts->l.d_partitions[i].p_size > 0)
			new_id++;
		if (info->nat_type->generic_ptype != PT_root &&
		    info->nat_type->generic_ptype != PT_swap && i < RAW_PART)
			continue;
		if (i == 0 && info->nat_type->generic_ptype != PT_root)
			continue;
		if (i == 1 && info->nat_type->generic_ptype != PT_swap)
			continue;
		if (i == RAW_PART)
			continue;
#if RAW_PART > 2
		if (i == RAW_PART-1)
			continue;
#endif
		if (parts->l.d_partitions[i].p_size > 0)
			continue;
		part = i;
		break;
	}

	if (part < 0) {
		if (parts->l.d_npartitions >= dl_maxpart) {
			if (err_msg)
				*err_msg =
				    msg_string(MSG_err_too_many_partitions);
			return NO_PART;
		}

		part = parts->l.d_npartitions++;
	}
	parts->l.d_partitions[part].p_offset = data.start;
	parts->l.d_partitions[part].p_size = data.size;
	parts->l.d_partitions[part].p_fstype =
	     dl_part_type_from_generic(info->nat_type);
	if (info->last_mounted && info->last_mounted[0])
		strlcpy(parts->last_mounted[part], info->last_mounted,
		    sizeof(parts->last_mounted[part]));
	else
		parts->last_mounted[part][0] = 0;
	parts->fs_sub_type[part] = info->fs_sub_type;
	parts->dp.num_part++;
	if (data.size <= parts->dp.free_space)
		parts->dp.free_space -= data.size;
	else
		parts->dp.free_space = 0;

	return new_id;
}

static part_id
disklabel_add_outer_partition(struct disk_partitions *arg,
    const struct disk_part_info *info, const char **err_msg)
{
	struct disklabel_disk_partitions *parts =
	    (struct disklabel_disk_partitions*)arg;
	int i, part = -1;
	part_id new_id;

	if (dl_maxpart == 0)
		dl_maxpart = getmaxpartitions();

	for (new_id = 0, i = 0; i < parts->l.d_npartitions; i++) {
		if (parts->l.d_partitions[i].p_size > 0)
			new_id++;
		if (info->nat_type->generic_ptype != PT_root &&
		    info->nat_type->generic_ptype != PT_swap && i < RAW_PART)
			continue;
		if (i == 0 && info->nat_type->generic_ptype != PT_root)
			continue;
		if (i == 1 && info->nat_type->generic_ptype != PT_swap)
			continue;
		if (i == RAW_PART)
			continue;
#if RAW_PART > 2
		if (i == RAW_PART-1)
			continue;
#endif
		if (parts->l.d_partitions[i].p_size > 0)
			continue;
		part = i;
		break;
	}

	if (part < 0) {
		if (parts->l.d_npartitions >= dl_maxpart) {
			if (err_msg)
				*err_msg =
				    msg_string(MSG_err_too_many_partitions);
			return NO_PART;
		}

		part = parts->l.d_npartitions++;
	}
	parts->l.d_partitions[part].p_offset = info->start;
	parts->l.d_partitions[part].p_size = info->size;
	parts->l.d_partitions[part].p_fstype =
	     dl_part_type_from_generic(info->nat_type);
	if (info->last_mounted && info->last_mounted[0])
		strlcpy(parts->last_mounted[part], info->last_mounted,
		    sizeof(parts->last_mounted[part]));
	else
		parts->last_mounted[part][0] = 0;
	parts->fs_sub_type[part] = info->fs_sub_type;
	parts->dp.num_part++;

	return new_id;
}

static size_t
disklabel_get_free_spaces(const struct disk_partitions *arg,
    struct disk_part_free_space *result, size_t max_num_result,
    daddr_t min_space_size, daddr_t align, daddr_t start, daddr_t ignore)
{
	const struct disklabel_disk_partitions *parts =
	    (const struct disklabel_disk_partitions*)arg;

	return disklabel_get_free_spaces_internal(parts, result,
	    max_num_result, min_space_size, align, start, ignore);
}

static daddr_t
disklabel_max_free_space_at(const struct disk_partitions *arg, daddr_t start)
{
	const struct disklabel_disk_partitions *parts =
	    (const struct disklabel_disk_partitions*)arg;
	struct disk_part_free_space space;

	if (disklabel_get_free_spaces_internal(parts, &space, 1, 1, 0,
	    start, start) == 1)
		return space.size;

	return 0;
}

static daddr_t
disklabel_get_alignment(const struct disk_partitions *arg)
{
	const struct disklabel_disk_partitions *parts =
	    (const struct disklabel_disk_partitions*)arg;

	return parts->ptn_alignment;
}

static part_id
disklabel_find_by_name(struct disk_partitions *arg, const char *name)
{
	const struct disklabel_disk_partitions *parts =
	    (const struct disklabel_disk_partitions*)arg;
	char *sl, part;
	ptrdiff_t n;
	part_id pno;

	sl = strrchr(name, '/');
	if (sl == NULL)
		return NO_PART;
	n = sl - name;
	if (strncmp(name, parts->l.d_packname, n) != 0)
		return NO_PART;
	part = name[n+1];
	if (part < 'a')
		return NO_PART;
	pno = part - 'a';
	if (pno >= parts->l.d_npartitions)
		return NO_PART;
	if (parts->l.d_partitions[pno].p_fstype == FS_UNUSED)
		return NO_PART;
	return pno;
}

static void
disklabel_free(struct disk_partitions *arg)
{

	assert(arg != NULL);
	free(arg);
}

const struct disk_partitioning_scheme
disklabel_parts = {
	.name = MSG_parttype_disklabel,
	.short_name = MSG_parttype_disklabel_short,
	.new_type_prompt = MSG_dl_get_custom_fstype,
	.size_limit = (daddr_t)UINT32_MAX,
	.write_to_disk = disklabel_write_to_disk,
	.read_from_disk = disklabel_parts_read,
	.create_new_for_disk = disklabel_parts_new,
	.change_disk_geom = disklabel_change_geom,
	.find_by_name = disklabel_find_by_name,
	.get_disk_pack_name = disklabel_get_disk_pack_name,
	.set_disk_pack_name = disklabel_set_disk_pack_name,
	.delete_all_partitions = disklabel_delete_all,
	.delete_partitions_in_range = disklabel_delete_range,
	.delete_partition = disklabel_delete,
	.get_part_types_count = disklabel_type_count,
	.get_part_type = disklabel_get_type,
	.get_generic_part_type = disklabel_get_generic_type,
	.get_fs_part_type = disklabel_get_fs_part_type,
	.create_custom_part_type = disklabel_create_custom_part_type,
	.get_part_alignment = disklabel_get_alignment,
	.get_part_info = disklabel_get_part_info,
	.can_add_partition = disklabel_can_add_partition,
	.set_part_info = disklabel_set_part_info,
	.add_partition = disklabel_add_partition,
	.add_outer_partition = disklabel_add_outer_partition,
	.max_free_space_at = disklabel_max_free_space_at,
	.get_free_spaces = disklabel_get_free_spaces,
	.get_part_device = disklabel_get_part_device,
	.free = disklabel_free,
};

/*	$NetBSD: disks.c,v 1.81 2022/06/02 15:36:08 martin Exp $ */

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
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

/* disks.c -- routines to deal with finding disks and labeling disks. */


#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <util.h>
#include <uuid.h>
#include <paths.h>
#include <fstab.h>

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/swap.h>
#include <sys/disklabel_gpt.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>

#include <dev/scsipi/scsipi_all.h>
#include <sys/scsiio.h>

#include <dev/ata/atareg.h>
#include <sys/ataio.h>

#include <sys/drvctlio.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"
#include "txtwalk.h"

/* #define DEBUG_VERBOSE	1 */

/* Disk descriptions */
struct disk_desc {
	char	dd_name[SSTRSIZE];
	char	dd_descr[256];
	bool	dd_no_mbr, dd_no_part;
	uint	dd_cyl;
	uint	dd_head;
	uint	dd_sec;
	uint	dd_secsize;
	daddr_t	dd_totsec;
};

#define	NAME_PREFIX	"NAME="
static const char name_prefix[] = NAME_PREFIX;

/* things we could have as /sbin/newfs_* and /sbin/fsck_* */
static const char *extern_fs_with_chk[] = {
	"ext2fs", "lfs", "msdos", "v7fs"
};

/* things we could have as /sbin/newfs_* but not /sbin/fsck_* */
static const char *extern_fs_newfs_only[] = {
	"sysvbfs", "udf"
};

/* Local prototypes */
static int found_fs(struct data *, size_t, const struct lookfor*);
static int found_fs_nocheck(struct data *, size_t, const struct lookfor*);
static int fsck_preen(const char *, const char *, bool silent);
static void fixsb(const char *, const char *);


static bool tmpfs_on_var_shm(void);

const char *
getfslabelname(uint f, uint f_version)
{
	if (f == FS_TMPFS)
		return "tmpfs";
	else if (f == FS_MFS)
		return "mfs";
	else if (f == FS_BSDFFS && f_version > 0)
		return f_version == 2 ?
		    msg_string(MSG_fs_type_ffsv2) : msg_string(MSG_fs_type_ffs);
	else if (f == FS_EX2FS && f_version == 1)
		return msg_string(MSG_fs_type_ext2old);
	else if (f >= __arraycount(fstypenames) || fstypenames[f] == NULL)
		return "invalid";
	return fstypenames[f];
}

/*
 * Decide wether we want to mount a tmpfs on /var/shm: we do this always
 * when the machine has more than 16 MB of user memory. On smaller machines,
 * shm_open() and friends will not perform well anyway.
 */
static bool
tmpfs_on_var_shm()
{
	uint64_t ram;
	size_t len;

	len = sizeof(ram);
	if (sysctlbyname("hw.usermem64", &ram, &len, NULL, 0))
		return false;

	return ram > 16 * MEG;
}

/* from src/sbin/atactl/atactl.c
 * extract_string: copy a block of bytes out of ataparams and make
 * a proper string out of it, truncating trailing spaces and preserving
 * strict typing. And also, not doing unaligned accesses.
 */
static void
ata_extract_string(char *buf, size_t bufmax,
		   uint8_t *bytes, unsigned numbytes,
		   int needswap)
{
	unsigned i;
	size_t j;
	unsigned char ch1, ch2;

	for (i = 0, j = 0; i < numbytes; i += 2) {
		ch1 = bytes[i];
		ch2 = bytes[i+1];
		if (needswap && j < bufmax-1) {
			buf[j++] = ch2;
		}
		if (j < bufmax-1) {
			buf[j++] = ch1;
		}
		if (!needswap && j < bufmax-1) {
			buf[j++] = ch2;
		}
	}
	while (j > 0 && buf[j-1] == ' ') {
		j--;
	}
	buf[j] = '\0';
}

/*
 * from src/sbin/scsictl/scsi_subr.c
 */
#define STRVIS_ISWHITE(x) ((x) == ' ' || (x) == '\0' || (x) == (u_char)'\377')

static void
scsi_strvis(char *sdst, size_t dlen, const char *ssrc, size_t slen)
{
	u_char *dst = (u_char *)sdst;
	const u_char *src = (const u_char *)ssrc;

	/* Trim leading and trailing blanks and NULs. */
	while (slen > 0 && STRVIS_ISWHITE(src[0]))
		++src, --slen;
	while (slen > 0 && STRVIS_ISWHITE(src[slen - 1]))
		--slen;

	while (slen > 0) {
		if (*src < 0x20 || *src >= 0x80) {
			/* non-printable characters */
			dlen -= 4;
			if (dlen < 1)
				break;
			*dst++ = '\\';
			*dst++ = ((*src & 0300) >> 6) + '0';
			*dst++ = ((*src & 0070) >> 3) + '0';
			*dst++ = ((*src & 0007) >> 0) + '0';
		} else if (*src == '\\') {
			/* quote characters */
			dlen -= 2;
			if (dlen < 1)
				break;
			*dst++ = '\\';
			*dst++ = '\\';
		} else {
			/* normal characters */
			if (--dlen < 1)
				break;
			*dst++ = *src;
		}
		++src, --slen;
	}

	*dst++ = 0;
}


static int
get_descr_scsi(struct disk_desc *dd)
{
	struct scsipi_inquiry_data inqbuf;
	struct scsipi_inquiry cmd;
	scsireq_t req;
        /* x4 in case every character is escaped, +1 for NUL. */
	char vendor[(sizeof(inqbuf.vendor) * 4) + 1],
	     product[(sizeof(inqbuf.product) * 4) + 1],
	     revision[(sizeof(inqbuf.revision) * 4) + 1];
	char size[5];

	memset(&inqbuf, 0, sizeof(inqbuf));
	memset(&cmd, 0, sizeof(cmd));
	memset(&req, 0, sizeof(req));

	cmd.opcode = INQUIRY;
	cmd.length = sizeof(inqbuf);
	memcpy(req.cmd, &cmd, sizeof(cmd));
	req.cmdlen = sizeof(cmd);
	req.databuf = &inqbuf;
	req.datalen = sizeof(inqbuf);
	req.timeout = 10000;
	req.flags = SCCMD_READ;
	req.senselen = SENSEBUFLEN;

	if (!disk_ioctl(dd->dd_name, SCIOCCOMMAND, &req)
	    || req.retsts != SCCMD_OK)
		return 0;

	scsi_strvis(vendor, sizeof(vendor), inqbuf.vendor,
	    sizeof(inqbuf.vendor));
	scsi_strvis(product, sizeof(product), inqbuf.product,
	    sizeof(inqbuf.product));
	scsi_strvis(revision, sizeof(revision), inqbuf.revision,
	    sizeof(inqbuf.revision));

	humanize_number(size, sizeof(size),
	    (uint64_t)dd->dd_secsize * (uint64_t)dd->dd_totsec,
	    "", HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);

	snprintf(dd->dd_descr, sizeof(dd->dd_descr),
	    "%s (%s, %s %s)",
	    dd->dd_name, size, vendor, product);

	return 1;
}

static int
get_descr_ata(struct disk_desc *dd)
{
	struct atareq req;
	static union {
		unsigned char inbuf[DEV_BSIZE];
		struct ataparams inqbuf;
	} inbuf;
	struct ataparams *inqbuf = &inbuf.inqbuf;
	char model[sizeof(inqbuf->atap_model)+1];
	char size[5];
	int needswap = 0;

	memset(&inbuf, 0, sizeof(inbuf));
	memset(&req, 0, sizeof(req));

	req.flags = ATACMD_READ;
	req.command = WDCC_IDENTIFY;
	req.databuf = (void *)&inbuf;
	req.datalen = sizeof(inbuf);
	req.timeout = 1000;

	if (!disk_ioctl(dd->dd_name, ATAIOCCOMMAND, &req)
	    || req.retsts != ATACMD_OK)
		return 0;

#if BYTE_ORDER == LITTLE_ENDIAN
	/*
	 * On little endian machines, we need to shuffle the string
	 * byte order.  However, we don't have to do this for NEC or
	 * Mitsumi ATAPI devices
	 */

	if (!(inqbuf->atap_config != WDC_CFG_CFA_MAGIC &&
	      (inqbuf->atap_config & WDC_CFG_ATAPI) &&
	      ((inqbuf->atap_model[0] == 'N' &&
	        inqbuf->atap_model[1] == 'E') ||
	       (inqbuf->atap_model[0] == 'F' &&
	        inqbuf->atap_model[1] == 'X')))) {
		needswap = 1;
	}
#endif

	ata_extract_string(model, sizeof(model),
	    inqbuf->atap_model, sizeof(inqbuf->atap_model), needswap);
	humanize_number(size, sizeof(size),
	    (uint64_t)dd->dd_secsize * (uint64_t)dd->dd_totsec,
	    "", HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);

	snprintf(dd->dd_descr, sizeof(dd->dd_descr), "%s (%s, %s)",
	    dd->dd_name, size, model);

	return 1;
}

static int
get_descr_drvctl(struct disk_desc *dd)
{
	prop_dictionary_t command_dict;
	prop_dictionary_t args_dict;
	prop_dictionary_t results_dict;
	prop_dictionary_t props;
	int8_t perr;
	int error, fd;
	bool rv;
	char size[5];
	const char *model;

	fd = open("/dev/drvctl", O_RDONLY);
	if (fd == -1)
		return 0;

	command_dict = prop_dictionary_create();
	args_dict = prop_dictionary_create();

	prop_dictionary_set_string_nocopy(command_dict, "drvctl-command",
	    "get-properties");
	prop_dictionary_set_string_nocopy(args_dict, "device-name",
	    dd->dd_name);
	prop_dictionary_set(command_dict, "drvctl-arguments", args_dict);
	prop_object_release(args_dict);

	error = prop_dictionary_sendrecv_ioctl(command_dict, fd,
	    DRVCTLCOMMAND, &results_dict);
	prop_object_release(command_dict);
	close(fd);
	if (error)
		return 0;

	rv = prop_dictionary_get_int8(results_dict, "drvctl-error", &perr);
	if (rv == false || perr != 0) {
		prop_object_release(results_dict);
		return 0;
	}

	props = prop_dictionary_get(results_dict,
	    "drvctl-result-data");
	if (props == NULL) {
		prop_object_release(results_dict);
		return 0;
	}
	props = prop_dictionary_get(props, "disk-info");
	if (props == NULL ||
	    !prop_dictionary_get_string(props, "type", &model)) {
		prop_object_release(results_dict);
		return 0;
	}

	humanize_number(size, sizeof(size),
	    (uint64_t)dd->dd_secsize * (uint64_t)dd->dd_totsec,
	    "", HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);

	snprintf(dd->dd_descr, sizeof(dd->dd_descr), "%s (%s, %s)",
	    dd->dd_name, size, model);

	prop_object_release(results_dict);

	return 1;
}

static void
get_descr(struct disk_desc *dd)
{
	char size[5];
	dd->dd_descr[0] = '\0';

	/* try drvctl first, fallback to direct probing */
	if (get_descr_drvctl(dd))
		return;
	/* try ATA */
	if (get_descr_ata(dd))
		return;
	/* try SCSI */
	if (get_descr_scsi(dd))
		return;

	/* XXX: get description from raid, cgd, vnd... */

	/* punt, just give some generic info */
	humanize_number(size, sizeof(size),
	    (uint64_t)dd->dd_secsize * (uint64_t)dd->dd_totsec,
	    "", HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);

	snprintf(dd->dd_descr, sizeof(dd->dd_descr),
	    "%s (%s)", dd->dd_name, size);
}

/*
 * State for helper callback for get_default_cdrom
 */
struct default_cdrom_data {
	char *device;
	size_t max_len;
	bool found;
};

/*
 * Helper function for get_default_cdrom, gets passed a device
 * name and a void pointer to default_cdrom_data.
 */
static bool
get_default_cdrom_helper(void *state, const char *dev)
{
	struct default_cdrom_data *data = state;

	if (!is_cdrom_device(dev, false))
		return true;

	strlcpy(data->device, dev, data->max_len);
	strlcat(data->device, "a", data->max_len); /* default to partition a */
	data->found = true;

	return false;	/* one is enough, stop iteration */
}

/*
 * Set the argument to the name of the first CD devices actually
 * available, leave it unmodified otherwise.
 * Return true if a device has been found.
 */
bool
get_default_cdrom(char *cd, size_t max_len)
{
	struct default_cdrom_data state;

	state.device = cd;
	state.max_len = max_len;
	state.found = false;

	if (enumerate_disks(&state, get_default_cdrom_helper))
		return state.found;

	return false;
}

static bool
get_wedge_descr(struct disk_desc *dd)
{
	struct dkwedge_info dkw;

	if (!get_wedge_info(dd->dd_name, &dkw))
		return false;

	snprintf(dd->dd_descr, sizeof(dd->dd_descr), "%s (%s@%s)",
	    dkw.dkw_wname, dkw.dkw_devname, dkw.dkw_parent);
	return true;
}

static bool
get_name_and_parent(const char *dev, char *name, char *parent)
{
	struct dkwedge_info dkw;

	if (!get_wedge_info(dev, &dkw))
		return false;
	strcpy(name, (const char *)dkw.dkw_wname);
	strcpy(parent, dkw.dkw_parent);
	return true;
}

static bool
find_swap_part_on(const char *dev, char *swap_name)
{
	struct dkwedge_list dkwl;
	struct dkwedge_info *dkw;
	u_int i;
	bool res = false;

	if (!get_wedge_list(dev, &dkwl))
		return false;

	dkw = dkwl.dkwl_buf;
	for (i = 0; i < dkwl.dkwl_nwedges; i++) {
		res = strcmp(dkw[i].dkw_ptype, DKW_PTYPE_SWAP) == 0;
		if (res) {
			strcpy(swap_name, (const char*)dkw[i].dkw_wname);
			break;
		}
	}
	free(dkwl.dkwl_buf);

	return res;
}

static bool
is_ffs_wedge(const char *dev)
{
	struct dkwedge_info dkw;

	if (!get_wedge_info(dev, &dkw))
		return false;

	return strcmp(dkw.dkw_ptype, DKW_PTYPE_FFS) == 0;
}

/*
 * Does this device match an entry in our default CDROM device list?
 * If looking for install targets, we also flag floopy devices.
 */
bool
is_cdrom_device(const char *dev, bool as_target)
{
	static const char *target_devices[] = {
#ifdef CD_NAMES
		CD_NAMES
#endif
#if defined(CD_NAMES) && defined(FLOPPY_NAMES)
		,
#endif
#ifdef FLOPPY_NAMES
		FLOPPY_NAMES
#endif
#if defined(CD_NAMES) || defined(FLOPPY_NAMES)
		,
#endif
		0
	};
	static const char *src_devices[] = {
#ifdef CD_NAMES
		CD_NAMES ,
#endif
		0
	};

	for (const char **dev_pat = as_target ? target_devices : src_devices;
	     *dev_pat; dev_pat++)
		if (fnmatch(*dev_pat, dev, 0) == 0)
			return true;

	return false;
}

/* does this device match any entry in the driver list? */
static bool
dev_in_list(const char *dev, const char **list)
{

	for ( ; *list; list++) {

		size_t len = strlen(*list);

		/* start of name matches? */
		if (strncmp(dev, *list, len) == 0) {
			char *endp;
			int e;

			/* remainder of name is a decimal number? */
			strtou(dev+len, &endp, 10, 0, INT_MAX, &e);
			if (endp && *endp == 0 && e == 0)
				return true;
		}
	}

	return false;
}

bool
is_bootable_device(const char *dev)
{
	static const char *non_bootable_devs[] = {
		"raid",	/* bootcode lives outside of raid */
		"xbd",	/* xen virtual device, can not boot from that */
		NULL
	};

	return !dev_in_list(dev, non_bootable_devs);
}

bool
is_partitionable_device(const char *dev)
{
	static const char *non_partitionable_devs[] = {
		"dk",	/* this is already a partitioned slice */
		NULL
	};

	return !dev_in_list(dev, non_partitionable_devs);
}

/*
 * Multi-purpose helper function:
 * iterate all known disks, invoke a callback for each.
 * Stop iteration when the callback returns false.
 * Return true when iteration actually happened, false on error.
 */
bool
enumerate_disks(void *state, bool (*func)(void *state, const char *dev))
{
	static const int mib[] = { CTL_HW, HW_DISKNAMES };
	static const unsigned int miblen = __arraycount(mib);
	const char *xd;
	char *disk_names;
	size_t len;

	if (sysctl(mib, miblen, NULL, &len, NULL, 0) == -1)
		return false;

	disk_names = malloc(len);
	if (disk_names == NULL)
		return false;

	if (sysctl(mib, miblen, disk_names, &len, NULL, 0) == -1) {
		free(disk_names);
		return false;
	}

	for (xd = strtok(disk_names, " "); xd != NULL; xd = strtok(NULL, " ")) {
		if (!(*func)(state, xd))
			break;
	}
	free(disk_names);

	return true;
}

/*
 * Helper state for get_disks
 */
struct get_disks_state {
	int numdisks;
	struct disk_desc *dd;
	bool with_non_partitionable;
};

/*
 * Helper function for get_disks enumartion
 */
static bool
get_disks_helper(void *arg, const char *dev)
{
	struct get_disks_state *state = arg;
	struct disk_geom geo;

	/* is this a CD device? */
	if (is_cdrom_device(dev, true))
		return true;

	memset(state->dd, 0, sizeof(*state->dd));
	strlcpy(state->dd->dd_name, dev, sizeof state->dd->dd_name - 2);
	state->dd->dd_no_mbr = !is_bootable_device(dev);
	state->dd->dd_no_part = !is_partitionable_device(dev);

	if (state->dd->dd_no_part && !state->with_non_partitionable)
		return true;

	if (!get_disk_geom(state->dd->dd_name, &geo)) {
		if (errno == ENOENT)
			return true;
		if (errno != ENOTTY || !state->dd->dd_no_part)
			/*
			 * Allow plain partitions,
			 * like already existing wedges
			 * (like dk0) if marked as
			 * non-partitioning device.
			 * For all other cases, continue
			 * with the next disk.
			 */
			return true;
		if (!is_ffs_wedge(state->dd->dd_name))
			return true;
	}

	/*
	 * Exclude a disk mounted as root partition,
	 * in case of install-image on a USB memstick.
	 */
	if (is_active_rootpart(state->dd->dd_name,
	    state->dd->dd_no_part ? -1 : 0))
		return true;

	state->dd->dd_cyl = geo.dg_ncylinders;
	state->dd->dd_head = geo.dg_ntracks;
	state->dd->dd_sec = geo.dg_nsectors;
	state->dd->dd_secsize = geo.dg_secsize;
	state->dd->dd_totsec = geo.dg_secperunit;

	if (!state->dd->dd_no_part || !get_wedge_descr(state->dd))
		get_descr(state->dd);
	state->dd++;
	state->numdisks++;
	if (state->numdisks == MAX_DISKS)
		return false;

	return true;
}

/*
 * Get all disk devices that are not CDs.
 * Optionally leave out those that can not be partitioned further.
 */
static int
get_disks(struct disk_desc *dd, bool with_non_partitionable)
{
	struct get_disks_state state;

	/* initialize */
	state.numdisks = 0;
	state.dd = dd;
	state.with_non_partitionable = with_non_partitionable;

	if (enumerate_disks(&state, get_disks_helper))
		return state.numdisks;

	return 0;
}

#ifdef DEBUG_VERBOSE
static void
dump_parts(const struct disk_partitions *parts)
{
	fprintf(stderr, "%s partitions on %s:\n",
	    MSG_XLAT(parts->pscheme->short_name), parts->disk);

	for (size_t p = 0; p < parts->num_part; p++) {
		struct disk_part_info info;

		if (parts->pscheme->get_part_info(
		    parts, p, &info)) {
			fprintf(stderr, " #%zu: start: %" PRIu64 " "
			    "size: %" PRIu64 ", flags: %x\n",
			    p, info.start, info.size,
			    info.flags);
			if (info.nat_type)
				fprintf(stderr, "\ttype: %s\n",
				    info.nat_type->description);
		} else {
			fprintf(stderr, "failed to get info "
			    "for partition #%zu\n", p);
		}
	}
	fprintf(stderr, "%" PRIu64 " sectors free, disk size %" PRIu64
	    " sectors, %zu partitions used\n", parts->free_space,
	    parts->disk_size, parts->num_part);
}
#endif

static bool
delete_scheme(struct pm_devs *p)
{

	if (!ask_noyes(MSG_removepartswarn))
		return false;

	p->parts->pscheme->free(p->parts);
	p->parts = NULL;
	return true;
}


static void
convert_copy(struct disk_partitions *old_parts,
    struct disk_partitions *new_parts)
{
	struct disk_part_info oinfo, ninfo;
	part_id i;

	for (i = 0; i < old_parts->num_part; i++) {
		if (!old_parts->pscheme->get_part_info(old_parts, i, &oinfo))
			continue;

		if (oinfo.flags & PTI_PSCHEME_INTERNAL)
			continue;

		if (oinfo.flags & PTI_SEC_CONTAINER) {
		    	if (old_parts->pscheme->secondary_partitions) {
				struct disk_partitions *sec_part =
					old_parts->pscheme->
					    secondary_partitions(
					    old_parts, oinfo.start, false);
				if (sec_part)
					convert_copy(sec_part, new_parts);
			}
			continue;
		}

		if (!new_parts->pscheme->adapt_foreign_part_info(new_parts,
			    &ninfo, old_parts->pscheme, &oinfo))
			continue;
		new_parts->pscheme->add_partition(new_parts, &ninfo, NULL);
	}
}

bool
convert_scheme(struct pm_devs *p, bool is_boot_drive, const char **err_msg)
{
	struct disk_partitions *old_parts, *new_parts;
	const struct disk_partitioning_scheme *new_scheme;

	*err_msg = NULL;

	old_parts = p->parts;
	new_scheme = select_part_scheme(p, old_parts->pscheme,
	    false, MSG_select_other_partscheme);

	if (new_scheme == NULL) {
		if (err_msg)
			*err_msg = INTERNAL_ERROR;
		return false;
	}

	new_parts = new_scheme->create_new_for_disk(p->diskdev,
	    0, p->dlsize, is_boot_drive, NULL);
	if (new_parts == NULL) {
		if (err_msg)
			*err_msg = MSG_out_of_memory;
		return false;
	}

	convert_copy(old_parts, new_parts);

	if (new_parts->num_part == 0 && old_parts->num_part != 0) {
		/* need to cleanup */
		new_parts->pscheme->free(new_parts);
		return false;
	}

	old_parts->pscheme->free(old_parts);
	p->parts = new_parts;
	return true;
}

static struct pm_devs *
dummy_whole_system_pm(void)
{
	static struct pm_devs whole_system = {
		.diskdev = "/",
		.no_mbr = true,
		.no_part = true,
		.cur_system = true,
	};
	static bool init = false;

	if (!init) {
		strlcpy(whole_system.diskdev_descr,
		    msg_string(MSG_running_system),
		    sizeof whole_system.diskdev_descr);
	}

	return &whole_system;
}

int
find_disks(const char *doingwhat, bool allow_cur_system)
{
	struct disk_desc disks[MAX_DISKS];
	/* need two more menu entries: current system + extended partitioning */
	menu_ent dsk_menu[__arraycount(disks) + 2],
	    wedge_menu[__arraycount(dsk_menu)];
	int disk_no[__arraycount(dsk_menu)], wedge_no[__arraycount(dsk_menu)];
	struct disk_desc *disk;
	int i = 0, dno, wno, skipped = 0;
	int already_found, numdisks, selected_disk = -1;
	int menu_no, w_menu_no;
	size_t max_desc_len;
	struct pm_devs *pm_i, *pm_last = NULL;
	bool any_wedges = false;

	memset(dsk_menu, 0, sizeof(dsk_menu));
	memset(wedge_menu, 0, sizeof(wedge_menu));

	/* Find disks. */
	numdisks = get_disks(disks, partman_go <= 0);

	/* need a redraw here, kernel messages hose everything */
	touchwin(stdscr);
	refresh();
	/* Kill typeahead, it won't be what the user had in mind */
	fpurge(stdin);
	/*
	 * we need space for the menu box and the row label,
	 * this sums up to 7 characters.
	 */
	max_desc_len = getmaxx(stdscr) - 8;
	if (max_desc_len >= __arraycount(disks[0].dd_descr))
		max_desc_len = __arraycount(disks[0].dd_descr) - 1;

	/*
	 * partman_go: <0 - we want to see menu with extended partitioning
	 *            ==0 - we want to see simple select disk menu
	 *             >0 - we do not want to see any menus, just detect
	 *                  all disks
	 */
	if (partman_go <= 0) {
		if (numdisks == 0 && !allow_cur_system) {
			/* No disks found! */
			hit_enter_to_continue(MSG_nodisk, NULL);
			/*endwin();*/
			return -1;
		} else {
			/* One or more disks found or current system allowed */
			dno = wno = 0;
			if (allow_cur_system) {
				dsk_menu[dno].opt_name = MSG_running_system;
				dsk_menu[dno].opt_flags = OPT_EXIT;
				dsk_menu[dno].opt_action = set_menu_select;
				disk_no[dno] = -1;
				i++; dno++;
			}
			for (i = 0; i < numdisks; i++) {
				if (disks[i].dd_no_part) {
					any_wedges = true;
					wedge_menu[wno].opt_name =
					    disks[i].dd_descr;
					disks[i].dd_descr[max_desc_len] = 0;
					wedge_menu[wno].opt_flags = OPT_EXIT;
					wedge_menu[wno].opt_action =
					    set_menu_select;
					wedge_no[wno] = i;
					wno++;
				} else {
					dsk_menu[dno].opt_name =
					    disks[i].dd_descr;
					disks[i].dd_descr[max_desc_len] = 0;
					dsk_menu[dno].opt_flags = OPT_EXIT;
					dsk_menu[dno].opt_action =
					    set_menu_select;
					disk_no[dno] = i;
					dno++;
				}
			}
			if (any_wedges) {
				dsk_menu[dno].opt_name = MSG_selectwedge;
				dsk_menu[dno].opt_flags = OPT_EXIT;
				dsk_menu[dno].opt_action = set_menu_select;
				disk_no[dno] = -2;
				dno++;
			}
			if (partman_go < 0) {
				dsk_menu[dno].opt_name = MSG_partman;
				dsk_menu[dno].opt_flags = OPT_EXIT;
				dsk_menu[dno].opt_action = set_menu_select;
				disk_no[dno] = -3;
				dno++;
			}
			w_menu_no = -1;
			menu_no = new_menu(MSG_Available_disks,
				dsk_menu, dno, -1,
				 4, 0, 0, MC_SCROLL,
				NULL, NULL, NULL, NULL, MSG_exit_menu_generic);
			if (menu_no == -1)
				return -1;
			for (;;) {
				msg_fmt_display(MSG_ask_disk, "%s", doingwhat);
				i = -1;
				process_menu(menu_no, &i);
				if (disk_no[i] == -2) {
					/* do wedges menu */
					if (w_menu_no == -1) {
						w_menu_no = new_menu(
						    MSG_Available_wedges,
						    wedge_menu, wno, -1,
						    4, 0, 0, MC_SCROLL,
						    NULL, NULL, NULL, NULL,
						    MSG_exit_menu_generic);
						if (w_menu_no == -1) {
							selected_disk = -1;
							break;
						}
					}
					i = -1;
					process_menu(w_menu_no, &i);
					if (i == -1)
						continue;
					selected_disk = wedge_no[i];
					break;
				}
				selected_disk = disk_no[i];
				break;
			}
			if (w_menu_no >= 0)
				free_menu(w_menu_no);
			free_menu(menu_no);
			if (allow_cur_system && selected_disk == -1) {
				pm = dummy_whole_system_pm();
				return 1;
			}
		}
		if (partman_go < 0 &&  selected_disk == -3) {
			partman_go = 1;
			return -2;
		} else
			partman_go = 0;
		if (selected_disk < 0 ||  selected_disk < 0
		    || selected_disk >= numdisks)
			return -1;
	}

	/* Fill pm struct with device(s) info */
	for (i = 0; i < numdisks; i++) {
		if (! partman_go)
			disk = disks + selected_disk;
		else {
			disk = disks + i;
			already_found = 0;
			SLIST_FOREACH(pm_i, &pm_head, l) {
				pm_last = pm_i;
				if (strcmp(pm_i->diskdev, disk->dd_name) == 0) {
					already_found = 1;
					break;
				}
			}
			if (pm_i != NULL && already_found) {
				/*
				 * We already added this device, but
				 * partitions might have changed
				 */
				if (!pm_i->found) {
					pm_i->found = true;
					if (pm_i->parts == NULL) {
						pm_i->parts =
						    partitions_read_disk(
						    pm_i->diskdev,
						    disk->dd_totsec,
						    disk->dd_secsize,
						    disk->dd_no_mbr);
					}
				}
				continue;
			}
		}
		pm = pm_new;
		pm->found = 1;
		pm->ptstart = 0;
		pm->ptsize = 0;
		strlcpy(pm->diskdev, disk->dd_name, sizeof pm->diskdev);
		strlcpy(pm->diskdev_descr, disk->dd_descr, sizeof pm->diskdev_descr);
		/* Use as a default disk if the user has the sets on a local disk */
		strlcpy(localfs_dev, disk->dd_name, sizeof localfs_dev);

		/*
		 * Init disk size and geometry
		 */
		pm->sectorsize = disk->dd_secsize;
		pm->dlcyl = disk->dd_cyl;
		pm->dlhead = disk->dd_head;
		pm->dlsec = disk->dd_sec;
		pm->dlsize = disk->dd_totsec;
		if (pm->dlsize == 0)
			pm->dlsize =
			    disk->dd_cyl * disk->dd_head * disk->dd_sec;

		pm->parts = partitions_read_disk(pm->diskdev,
		    pm->dlsize, disk->dd_secsize, disk->dd_no_mbr);

again:

#ifdef DEBUG_VERBOSE
		if (pm->parts) {
			fputs("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n", stderr);
			dump_parts(pm->parts);

			if (pm->parts->pscheme->secondary_partitions) {
				const struct disk_partitions *sparts =
				    pm->parts->pscheme->secondary_partitions(
				    pm->parts, pm->ptstart, false);
				if (sparts != NULL)
					dump_parts(sparts);
			}
		}
#endif

		pm->no_mbr = disk->dd_no_mbr;
		pm->no_part = disk->dd_no_part;
		if (!pm->no_part) {
			pm->sectorsize = disk->dd_secsize;
			pm->dlcyl = disk->dd_cyl;
			pm->dlhead = disk->dd_head;
			pm->dlsec = disk->dd_sec;
			pm->dlsize = disk->dd_totsec;
			if (pm->dlsize == 0)
				pm->dlsize =
				    disk->dd_cyl * disk->dd_head * disk->dd_sec;

			if (pm->parts && pm->parts->pscheme->size_limit != 0
			    && pm->dlsize > pm->parts->pscheme->size_limit
			    && ! partman_go) {

				char size[5], limit[5];

				humanize_number(size, sizeof(size),
				    (uint64_t)pm->dlsize * pm->sectorsize,
				    "", HN_AUTOSCALE, HN_B | HN_NOSPACE
				    | HN_DECIMAL);

				humanize_number(limit, sizeof(limit),
				    (uint64_t)pm->parts->pscheme->size_limit
					* 512U,
				    "", HN_AUTOSCALE, HN_B | HN_NOSPACE
				    | HN_DECIMAL);

				if (logfp)
					fprintf(logfp,
					    "disk %s: is too big (%" PRIu64
					    " blocks, %s), will be truncated\n",
						pm->diskdev, pm->dlsize,
						size);

				msg_display_subst(MSG_toobigdisklabel, 5,
				   pm->diskdev,
				   msg_string(pm->parts->pscheme->name),
				   msg_string(pm->parts->pscheme->short_name),
				   size, limit);

				int sel = -1;
				const char *err = NULL;
				process_menu(MENU_convertscheme, &sel);
				if (sel == 1) {
					if (!delete_scheme(pm)) {
						return -1;
					}
					goto again;
				} else if (sel == 2) {
					if (!convert_scheme(pm,
					     partman_go < 0, &err)) {
						if (err != NULL)
							err_msg_win(err);
						return -1;
					}
					goto again;
				} else if (sel == 3) {
					return -1;
				}
				pm->dlsize = pm->parts->pscheme->size_limit;
			}
		} else {
			pm->sectorsize = 0;
			pm->dlcyl = 0;
			pm->dlhead = 0;
			pm->dlsec = 0;
			pm->dlsize = 0;
			pm->no_mbr = 1;
		}
		pm->dlcylsize = pm->dlhead * pm->dlsec;

		if (partman_go) {
			pm_getrefdev(pm_new);
			if (SLIST_EMPTY(&pm_head) || pm_last == NULL)
				 SLIST_INSERT_HEAD(&pm_head, pm_new, l);
			else
				 SLIST_INSERT_AFTER(pm_last, pm_new, l);
			pm_new = malloc(sizeof (struct pm_devs));
			memset(pm_new, 0, sizeof *pm_new);
		} else
			/* We are not in partman and do not want to process
			 * all devices, exit */
			break;
	}

	return numdisks-skipped;
}

static int
sort_part_usage_by_mount(const void *a, const void *b)
{
	const struct part_usage_info *pa = a, *pb = b;

	/* sort all real partitions by mount point */
	if ((pa->instflags & PUIINST_MOUNT) &&
	    (pb->instflags & PUIINST_MOUNT))
		return strcmp(pa->mount, pb->mount);

	/* real partitions go first */
	if (pa->instflags & PUIINST_MOUNT)
		return -1;
	if (pb->instflags & PUIINST_MOUNT)
		return 1;

	/* arbitrary order for all other partitions */
	if (pa->type == PT_swap)
		return -1;
	if (pb->type == PT_swap)
		return 1;
	if (pa->type < pb->type)
		return -1;
	if (pa->type > pb->type)
		return 1;
	if (pa->cur_part_id < pb->cur_part_id)
		return -1;
	if (pa->cur_part_id > pb->cur_part_id)
		return 1;
	return (uintptr_t)a < (uintptr_t)b ? -1 : 1;
}

int
make_filesystems(struct install_partition_desc *install)
{
	int error = 0, partno = -1;
	char *newfs = NULL, devdev[PATH_MAX], rdev[PATH_MAX],
	    opts[200], opt[30];
	size_t i;
	struct part_usage_info *ptn;
	struct disk_partitions *parts;
	const char *mnt_opts = NULL, *fsname = NULL;

	if (pm->cur_system)
		return 1;

	if (pm->no_part) {
		/* check if this target device already has a ffs */
		snprintf(rdev, sizeof rdev, _PATH_DEV "/r%s", pm->diskdev);
		error = fsck_preen(rdev, "ffs", true);
		if (error) {
			if (!ask_noyes(MSG_No_filesystem_newfs))
				return EINVAL;
			error = run_program(RUN_DISPLAY | RUN_PROGRESS,
			    "/sbin/newfs -V2 -O2 %s", rdev);
		}

		md_pre_mount(install, 0);

		make_target_dir("/");

		snprintf(devdev, sizeof devdev, _PATH_DEV "%s", pm->diskdev);
		error = target_mount_do("-o async", devdev, "/");
		if (error) {
			msg_display_subst(MSG_mountfail, 2, devdev, "/");
			hit_enter_to_continue(NULL, NULL);
		}

		return error;
	}

	/* Making new file systems and mounting them */

	/* sort to ensure /usr/local is mounted after /usr (etc) */
	qsort(install->infos, install->num, sizeof(*install->infos),
	    sort_part_usage_by_mount);

	for (i = 0; i < install->num; i++) {
		/*
		 * Newfs all file systems marked as needing this.
		 * Mount the ones that have a mountpoint in the target.
		 */
		ptn = &install->infos[i];
		parts = ptn->parts;
		newfs = NULL;
		fsname = NULL;

		if (ptn->size == 0 || parts == NULL|| ptn->type == PT_swap)
			continue;

		if (parts->pscheme->get_part_device(parts, ptn->cur_part_id,
		    devdev, sizeof devdev, &partno, parent_device_only, false,
		    false) && is_active_rootpart(devdev, partno))
			continue;

		parts->pscheme->get_part_device(parts, ptn->cur_part_id,
		    devdev, sizeof devdev, &partno, plain_name, true, true);

		parts->pscheme->get_part_device(parts, ptn->cur_part_id,
		    rdev, sizeof rdev, &partno, raw_dev_name, true, true);

		opts[0] = 0;
		switch (ptn->fs_type) {
		case FS_APPLEUFS:
			if (ptn->fs_opt3 != 0)
				snprintf(opts, sizeof opts, "-i %u",
				    ptn->fs_opt3);
			asprintf(&newfs, "/sbin/newfs %s", opts);
			mnt_opts = "-tffs -o async";
			fsname = "ffs";
			break;
		case FS_BSDFFS:
			if (ptn->fs_opt3 != 0)
				snprintf(opts, sizeof opts, "-i %u ",
				    ptn->fs_opt3);
			if (ptn->fs_opt1 != 0) {
				snprintf(opt, sizeof opt, "-b %u ",
				    ptn->fs_opt1);
				strcat(opts, opt);
			}
			if (ptn->fs_opt2 != 0) {
				snprintf(opt, sizeof opt, "-f %u ",
				    ptn->fs_opt2);
				strcat(opts, opt);
			}
			asprintf(&newfs,
			    "/sbin/newfs -V2 -O %d %s",
			    ptn->fs_version == 2 ? 2 : 1, opts);
			if (ptn->mountflags & PUIMNT_LOG)
				mnt_opts = "-tffs -o log";
			else
				mnt_opts = "-tffs -o async";
			fsname = "ffs";
			break;
		case FS_BSDLFS:
			if (ptn->fs_opt1 != 0 && ptn->fs_opt2 != 0)
				snprintf(opts, sizeof opts, "-b %u",
				     ptn->fs_opt1 * ptn->fs_opt2);
			asprintf(&newfs, "/sbin/newfs_lfs %s", opts);
			mnt_opts = "-tlfs";
			fsname = "lfs";
			break;
		case FS_MSDOS:
			asprintf(&newfs, "/sbin/newfs_msdos");
			mnt_opts = "-tmsdos";
			fsname = "msdos";
			break;
		case FS_SYSVBFS:
			asprintf(&newfs, "/sbin/newfs_sysvbfs");
			mnt_opts = "-tsysvbfs";
			fsname = "sysvbfs";
			break;
		case FS_V7:
			asprintf(&newfs, "/sbin/newfs_v7fs");
			mnt_opts = "-tv7fs";
			fsname = "v7fs";
			break;
		case FS_EX2FS:
			asprintf(&newfs,
			    ptn->fs_version == 1 ?
				"/sbin/newfs_ext2fs -O 0" :
				"/sbin/newfs_ext2fs");
			mnt_opts = "-text2fs";
			fsname = "ext2fs";
			break;
		}
		if ((ptn->instflags & PUIINST_NEWFS) && newfs != NULL) {
			error = run_program(RUN_DISPLAY | RUN_PROGRESS,
			    "%s %s", newfs, rdev);
		} else if ((ptn->instflags & (PUIINST_MOUNT|PUIINST_BOOT))
		    && fsname != NULL) {
			/* We'd better check it isn't dirty */
			error = fsck_preen(devdev, fsname, false);
		}
		free(newfs);
		if (error != 0)
			return error;

		ptn->instflags &= ~PUIINST_NEWFS;
		md_pre_mount(install, i);

		if (partman_go == 0 && (ptn->instflags & PUIINST_MOUNT) &&
				mnt_opts != NULL) {
			make_target_dir(ptn->mount);
			error = target_mount_do(mnt_opts, devdev,
			    ptn->mount);
			if (error) {
				msg_display_subst(MSG_mountfail, 2, devdev,
				    ptn->mount);
				hit_enter_to_continue(NULL, NULL);
				return error;
			}
		}
	}
	return 0;
}

int
make_fstab(struct install_partition_desc *install)
{
	FILE *f;
	const char *dump_dev = NULL;
	const char *dev;
	char dev_buf[PATH_MAX], swap_dev[PATH_MAX];

	if (pm->cur_system)
		return 1;

	swap_dev[0] = 0;

	/* Create the fstab. */
	make_target_dir("/etc");
	f = target_fopen("/etc/fstab", "w");
	scripting_fprintf(NULL, "cat <<EOF >%s/etc/fstab\n", target_prefix());

	if (logfp)
		(void)fprintf(logfp,
		    "Making %s/etc/fstab (%s).\n", target_prefix(),
		    pm->diskdev);

	if (f == NULL) {
		msg_display(MSG_createfstab);
		if (logfp)
			(void)fprintf(logfp, "Failed to make /etc/fstab!\n");
		hit_enter_to_continue(NULL, NULL);
#ifndef DEBUG
		return 1;
#else
		f = stdout;
#endif
	}

	scripting_fprintf(f, "# NetBSD /etc/fstab\n# See /usr/share/examples/"
			"fstab/ for more examples.\n");

	if (pm->no_part) {
		/* single dk? target */
		char buf[200], parent[200], swap[200], *prompt;
		int res;

		if (!get_name_and_parent(pm->diskdev, buf, parent))
			goto done_with_disks;
		scripting_fprintf(f, NAME_PREFIX "%s\t/\tffs\trw\t\t1 1\n",
		    buf);
		if (!find_swap_part_on(parent, swap))
			goto done_with_disks;
		const char *args[] = { parent, swap };
		prompt = str_arg_subst(msg_string(MSG_Auto_add_swap_part),
		    __arraycount(args), args);
		res = ask_yesno(prompt);
		free(prompt);
		if (res)
			scripting_fprintf(f, NAME_PREFIX "%s\tnone"
			    "\tswap\tsw,dp\t\t0 0\n", swap);
		goto done_with_disks;
	}

	for (size_t i = 0; i < install->num; i++) {

		const struct part_usage_info *ptn = &install->infos[i];

		if (ptn->size == 0)
			continue;

		bool is_tmpfs = ptn->type == PT_root &&
		    ptn->fs_type == FS_TMPFS &&
		    (ptn->flags & PUIFLG_JUST_MOUNTPOINT);

		if (!is_tmpfs && ptn->type != PT_swap &&
		    (ptn->instflags & PUIINST_MOUNT) == 0)
			continue;

		const char *s = "";
		const char *mp = ptn->mount;
		const char *fstype = "ffs";
		int fsck_pass = 0, dump_freq = 0;

		if (ptn->parts->pscheme->get_part_device(ptn->parts,
			    ptn->cur_part_id, dev_buf, sizeof dev_buf, NULL,
			    logical_name, true, false))
			dev = dev_buf;
		else
			dev = NULL;

		if (!*mp) {
			/*
			 * No mount point specified, comment out line and
			 * use /mnt as a placeholder for the mount point.
			 */
			s = "# ";
			mp = "/mnt";
		}

		switch (ptn->fs_type) {
		case FS_UNUSED:
			continue;
		case FS_BSDLFS:
			/* If there is no LFS, just comment it out. */
			if (!check_lfs_progs())
				s = "# ";
			fstype = "lfs";
			/* FALLTHROUGH */
		case FS_BSDFFS:
			fsck_pass = (strcmp(mp, "/") == 0) ? 1 : 2;
			dump_freq = 1;
			break;
		case FS_MSDOS:
			fstype = "msdos";
			break;
		case FS_SWAP:
			if (swap_dev[0] == 0) {
				strlcpy(swap_dev, dev, sizeof swap_dev);
				dump_dev = ",dp";
			} else {
				dump_dev = "";
			}
			scripting_fprintf(f, "%s\t\tnone\tswap\tsw%s\t\t 0 0\n",
				dev, dump_dev);
			continue;
#ifdef HAVE_TMPFS
		case FS_TMPFS:
			if (ptn->size < 0)
				scripting_fprintf(f,
				    "tmpfs\t\t/tmp\ttmpfs\trw,-m=1777,"
				    "-s=ram%%%" PRIu64 "\n", -ptn->size);
			else
				scripting_fprintf(f,
				    "tmpfs\t\t/tmp\ttmpfs\trw,-m=1777,"
				    "-s=%" PRIu64 "M\n", ptn->size);
			continue;
#else
		case FS_MFS:
			if (swap_dev[0] != 0)
				scripting_fprintf(f,
				    "%s\t\t/tmp\tmfs\trw,-s=%"
				    PRIu64 "\n", swap_dev, ptn->size);
			else
				scripting_fprintf(f,
				    "swap\t\t/tmp\tmfs\trw,-s=%"
				    PRIu64 "\n", ptn->size);
			continue;
#endif
		case FS_SYSVBFS:
			fstype = "sysvbfs";
			make_target_dir("/stand");
			break;
		default:
			fstype = "???";
			s = "# ";
			break;
		}
		/* The code that remounts root rw doesn't check the partition */
		if (strcmp(mp, "/") == 0 &&
		    (ptn->instflags & PUIINST_MOUNT) == 0)
			s = "# ";

 		scripting_fprintf(f,
		  "%s%s\t\t%s\t%s\trw%s%s%s%s%s%s%s%s\t\t %d %d\n",
		   s, dev, mp, fstype,
		   ptn->mountflags & PUIMNT_LOG ? ",log" : "",
		   ptn->mountflags & PUIMNT_NOAUTO ? ",noauto" : "",
		   ptn->mountflags & PUIMNT_ASYNC ? ",async" : "",
		   ptn->mountflags & PUIMNT_NOATIME ? ",noatime" : "",
		   ptn->mountflags & PUIMNT_NODEV ? ",nodev" : "",
		   ptn->mountflags & PUIMNT_NODEVMTIME ? ",nodevmtime" : "",
		   ptn->mountflags & PUIMNT_NOEXEC ? ",noexec" : "",
		   ptn->mountflags & PUIMNT_NOSUID ? ",nosuid" : "",
		   dump_freq, fsck_pass);
	}

done_with_disks:
	if (cdrom_dev[0] == 0)
		get_default_cdrom(cdrom_dev, sizeof(cdrom_dev));

	/* Add /kern, /proc and /dev/pts to fstab and make mountpoint. */
	scripting_fprintf(f, "kernfs\t\t/kern\tkernfs\trw\n");
	scripting_fprintf(f, "ptyfs\t\t/dev/pts\tptyfs\trw\n");
	scripting_fprintf(f, "procfs\t\t/proc\tprocfs\trw\n");
	if (cdrom_dev[0] != 0)
		scripting_fprintf(f, "/dev/%s\t\t/cdrom\tcd9660\tro,noauto\n",
		    cdrom_dev);
	scripting_fprintf(f, "%stmpfs\t\t/var/shm\ttmpfs\trw,-m1777,-sram%%25\n",
	    tmpfs_on_var_shm() ? "" : "#");
	make_target_dir("/kern");
	make_target_dir("/proc");
	make_target_dir("/dev/pts");
	if (cdrom_dev[0] != 0)
		make_target_dir("/cdrom");
	make_target_dir("/var/shm");

	scripting_fprintf(NULL, "EOF\n");

	fclose(f);
	fflush(NULL);
	return 0;
}

static bool
find_part_by_name(const char *name, struct disk_partitions **parts,
    part_id *pno)
{
	struct pm_devs *i;
	struct disk_partitions *ps;
	part_id id;
	struct disk_desc disks[MAX_DISKS];
	int n, cnt;

	if (SLIST_EMPTY(&pm_head)) {
		/*
		 * List has not been filled, only "pm" is valid - check
		 * that first.
		 */
		if (pm->parts != NULL &&
		    pm->parts->pscheme->find_by_name != NULL) {
			id = pm->parts->pscheme->find_by_name(pm->parts, name);
			if (id != NO_PART) {
				*pno = id;
				*parts = pm->parts;
				return true;
			}
		}
		/*
		 * Not that easy - check all other disks
		 */
		cnt = get_disks(disks, false);
		for (n = 0; n < cnt; n++) {
			if (strcmp(disks[n].dd_name, pm->diskdev) == 0)
				continue;
			ps = partitions_read_disk(disks[n].dd_name,
			    disks[n].dd_totsec,
			    disks[n].dd_secsize,
			    disks[n].dd_no_mbr);
			if (ps == NULL)
				continue;
			if (ps->pscheme->find_by_name == NULL)
				continue;
			id = ps->pscheme->find_by_name(ps, name);
			if (id != NO_PART) {
				*pno = id;
				*parts = ps;
				return true;	/* XXX this leaks memory */
			}
			ps->pscheme->free(ps);
		}
	} else {
		SLIST_FOREACH(i, &pm_head, l) {
			if (i->parts == NULL)
				continue;
			if (i->parts->pscheme->find_by_name == NULL)
				continue;
			id = i->parts->pscheme->find_by_name(i->parts, name);
			if (id == NO_PART)
				continue;
			*pno = id;
			*parts = i->parts;
			return true;
		}
	}

	*pno = NO_PART;
	*parts = NULL;
	return false;
}

static int
/*ARGSUSED*/
process_found_fs(struct data *list, size_t num, const struct lookfor *item,
    bool with_fsck)
{
	int error;
	char rdev[PATH_MAX], dev[PATH_MAX],
	    options[STRSIZE], tmp[STRSIZE], *op, *last;
	const char *fsname = (const char*)item->var;
	part_id pno;
	struct disk_partitions *parts;
	size_t len;
	bool first, is_root;

	if (num < 2 || strstr(list[2].u.s_val, "noauto") != NULL)
		return 0;

	is_root = strcmp(list[1].u.s_val, "/") == 0;
	if (is_root && target_mounted())
		return 0;

	if (strcmp(item->head, name_prefix) == 0) {
		/* this fstab entry uses NAME= syntax */

		/* unescape */
		char *src, *dst;
		for (src = list[0].u.s_val, dst =src; src[0] != 0; ) {
			if (src[0] == '\\' && src[1] != 0)
				src++;
			*dst++ = *src++;
		}
		*dst = 0;

		if (!find_part_by_name(list[0].u.s_val,
		    &parts, &pno) || parts == NULL || pno == NO_PART)
			return 0;
		parts->pscheme->get_part_device(parts, pno,
		    dev, sizeof(dev), NULL, plain_name, true, true);
		parts->pscheme->get_part_device(parts, pno,
		    rdev, sizeof(rdev), NULL, raw_dev_name, true, true);
	} else {
		/* this fstab entry uses the plain device name */
		if (is_root) {
			/*
			 * PR 54480: we can not use the current device name
			 * as it might be different from the real environment.
			 * This is an abuse of the functionality, but it used
			 * to work before (and still does work if only a single
			 * target disk is involved).
			 * Use the device name from the current "pm" instead.
			 */
			strcpy(rdev, "/dev/r");
			strlcat(rdev, pm->diskdev, sizeof(rdev));
			strcpy(dev, "/dev/");
			strlcat(dev, pm->diskdev, sizeof(dev));
			/* copy over the partition letter, if any */
			len = strlen(list[0].u.s_val);
			if (list[0].u.s_val[len-1] >= 'a' &&
			    list[0].u.s_val[len-1] <=
			    ('a' + getmaxpartitions())) {
				strlcat(rdev, &list[0].u.s_val[len-1],
				    sizeof(rdev));
				strlcat(dev, &list[0].u.s_val[len-1],
				    sizeof(dev));
			}
		} else {
			strcpy(rdev, "/dev/r");
			strlcat(rdev, list[0].u.s_val, sizeof(rdev));
			strcpy(dev, "/dev/");
			strlcat(dev, list[0].u.s_val, sizeof(dev));
		}
	}

	if (with_fsck) {
		/* need the raw device for fsck_preen */
		error = fsck_preen(rdev, fsname, false);
		if (error != 0)
			return error;
	}

	/* add mount option for fs type */
	strcpy(options, "-t ");
	strlcat(options, fsname, sizeof(options));

	/* extract mount options from fstab */
	strlcpy(tmp, list[2].u.s_val, sizeof(tmp));
	for (first = true, op = strtok_r(tmp, ",", &last); op != NULL;
	    op = strtok_r(NULL, ",", &last)) {
		if (strcmp(op, FSTAB_RW) == 0 ||
		    strcmp(op, FSTAB_RQ) == 0 ||
		    strcmp(op, FSTAB_RO) == 0 ||
		    strcmp(op, FSTAB_SW) == 0 ||
		    strcmp(op, FSTAB_DP) == 0 ||
		    strcmp(op, FSTAB_XX) == 0)
			continue;
		if (first) {
			first = false;
			strlcat(options, " -o ", sizeof(options));
		} else {
			strlcat(options, ",", sizeof(options));
		}
		strlcat(options, op, sizeof(options));
	}

	error = target_mount(options, dev, list[1].u.s_val);
	if (error != 0) {
		msg_fmt_display(MSG_mount_failed, "%s", list[0].u.s_val);
		if (!ask_noyes(NULL))
			return error;
	}
	return 0;
}

static int
/*ARGSUSED*/
found_fs(struct data *list, size_t num, const struct lookfor *item)
{
	return process_found_fs(list, num, item, true);
}

static int
/*ARGSUSED*/
found_fs_nocheck(struct data *list, size_t num, const struct lookfor *item)
{
	return process_found_fs(list, num, item, false);
}

/*
 * Do an fsck. On failure, inform the user by showing a warning
 * message and doing menu_ok() before proceeding.
 * The device passed should be the full qualified path to raw disk
 * (e.g. /dev/rwd0a).
 * Returns 0 on success, or nonzero return code from fsck() on failure.
 */
static int
fsck_preen(const char *disk, const char *fsname, bool silent)
{
	char *prog, err[12];
	int error;

	if (fsname == NULL)
		return 0;
	/* first, check if fsck program exists, if not, assume ok */
	asprintf(&prog, "/sbin/fsck_%s", fsname);
	if (prog == NULL)
		return 0;
	if (access(prog, X_OK) != 0) {
		free(prog);
		return 0;
	}
	if (!strcmp(fsname,"ffs"))
		fixsb(prog, disk);
	error = run_program(silent? RUN_SILENT|RUN_ERROR_OK : 0, "%s -p -q %s", prog, disk);
	free(prog);
	if (error != 0 && !silent) {
		sprintf(err, "%d", error);
		msg_display_subst(msg_string(MSG_badfs), 3,
		    disk, fsname, err);
		if (ask_noyes(NULL))
			error = 0;
		/* XXX at this point maybe we should run a full fsck? */
	}
	return error;
}

/* This performs the same function as the etc/rc.d/fixsb script
 * which attempts to correct problems with ffs1 filesystems
 * which may have been introduced by booting a netbsd-current kernel
 * from between April of 2003 and January 2004. For more information
 * This script was developed as a response to NetBSD pr install/25138
 * Additional prs regarding the original issue include:
 *  bin/17910 kern/21283 kern/21404 port-macppc/23925 port-macppc/23926
 */
static void
fixsb(const char *prog, const char *disk)
{
	int fd;
	int rval;
	union {
		struct fs fs;
		char buf[SBLOCKSIZE];
	} sblk;
	struct fs *fs = &sblk.fs;

	fd = open(disk, O_RDONLY);
	if (fd == -1)
		return;

	/* Read ffsv1 main superblock */
	rval = pread(fd, sblk.buf, sizeof sblk.buf, SBLOCK_UFS1);
	close(fd);
	if (rval != sizeof sblk.buf)
		return;

	if (fs->fs_magic != FS_UFS1_MAGIC &&
	    fs->fs_magic != FS_UFS1_MAGIC_SWAPPED)
		/* Not FFSv1 */
		return;
	if (fs->fs_old_flags & FS_FLAGS_UPDATED)
		/* properly updated fslevel 4 */
		return;
	if (fs->fs_bsize != fs->fs_maxbsize)
		/* not messed up */
		return;

	/*
	 * OK we have a munged fs, first 'upgrade' to fslevel 4,
	 * We specify -b16 in order to stop fsck bleating that the
	 * sb doesn't match the first alternate.
	 */
	run_program(RUN_DISPLAY | RUN_PROGRESS,
	    "%s -p -b 16 -c 4 %s", prog, disk);
	/* Then downgrade to fslevel 3 */
	run_program(RUN_DISPLAY | RUN_PROGRESS,
	    "%s -p -c 3 %s", prog, disk);
}

/*
 * fsck and mount the root partition.
 * devdev is the fully qualified block device name.
 */
static int
mount_root(const char *devdev, bool first, bool writeable,
     struct install_partition_desc *install)
{
	int	error;

	error = fsck_preen(devdev, "ffs", false);
	if (error != 0)
		return error;

	if (first)
		md_pre_mount(install, 0);

	/* Mount devdev on target's "".
	 * If we pass "" as mount-on, Prefixing will DTRT.
	 * for now, use no options.
	 * XXX consider -o remount in case target root is
	 * current root, still readonly from single-user?
	 */
	return target_mount(writeable? "" : "-r", devdev, "");
}

/* Get information on the file systems mounted from the root filesystem.
 * Offer to convert them into 4.4BSD inodes if they are not 4.4BSD
 * inodes.  Fsck them.  Mount them.
 */

int
mount_disks(struct install_partition_desc *install)
{
	char *fstab;
	int   fstabsize;
	int   error;
	char devdev[PATH_MAX];
	size_t i, num_fs_types, num_entries;
	struct lookfor *fstabbuf, *l;

	if (install->cur_system)
		return 0;

	/*
	 * Check what file system tools are available and create parsers
	 * for the corresponding fstab(5) entries - all others will be
	 * ignored.
	 */
	num_fs_types = 1;	/* ffs is implicit */
	for (i = 0; i < __arraycount(extern_fs_with_chk); i++) {
		sprintf(devdev, "/sbin/newfs_%s", extern_fs_with_chk[i]);
		if (file_exists_p(devdev))
			num_fs_types++;
	}
	for (i = 0; i < __arraycount(extern_fs_newfs_only); i++) {
		sprintf(devdev, "/sbin/newfs_%s", extern_fs_newfs_only[i]);
		if (file_exists_p(devdev))
			num_fs_types++;
	}
	num_entries = 2 *  num_fs_types + 1;	/* +1 for "ufs" special case */
	fstabbuf = calloc(num_entries, sizeof(*fstabbuf));
	if (fstabbuf == NULL)
		return -1;
	l = fstabbuf;
	l->head = "/dev/";
	l->fmt = strdup("/dev/%s %s ffs %s");
	l->todo = "c";
	l->var = __UNCONST("ffs");
	l->func = found_fs;
	l++;
	l->head = "/dev/";
	l->fmt = strdup("/dev/%s %s ufs %s");
	l->todo = "c";
	l->var = __UNCONST("ffs");
	l->func = found_fs;
	l++;
	l->head = NAME_PREFIX;
	l->fmt = strdup(NAME_PREFIX "%s %s ffs %s");
	l->todo = "c";
	l->var = __UNCONST("ffs");
	l->func = found_fs;
	l++;
	for (i = 0; i < __arraycount(extern_fs_with_chk); i++) {
		sprintf(devdev, "/sbin/newfs_%s", extern_fs_with_chk[i]);
		if (!file_exists_p(devdev))
			continue;
		sprintf(devdev, "/dev/%%s %%s %s %%s", extern_fs_with_chk[i]);
		l->head = "/dev/";
		l->fmt = strdup(devdev);
		l->todo = "c";
		l->var = __UNCONST(extern_fs_with_chk[i]);
		l->func = found_fs;
		l++;
		sprintf(devdev, NAME_PREFIX "%%s %%s %s %%s",
		    extern_fs_with_chk[i]);
		l->head = NAME_PREFIX;
		l->fmt = strdup(devdev);
		l->todo = "c";
		l->var = __UNCONST(extern_fs_with_chk[i]);
		l->func = found_fs;
		l++;
	}
	for (i = 0; i < __arraycount(extern_fs_newfs_only); i++) {
		sprintf(devdev, "/sbin/newfs_%s", extern_fs_newfs_only[i]);
		if (!file_exists_p(devdev))
			continue;
		sprintf(devdev, "/dev/%%s %%s %s %%s", extern_fs_newfs_only[i]);
		l->head = "/dev/";
		l->fmt = strdup(devdev);
		l->todo = "c";
		l->var = __UNCONST(extern_fs_newfs_only[i]);
		l->func = found_fs_nocheck;
		l++;
		sprintf(devdev, NAME_PREFIX "%%s %%s %s %%s",
		    extern_fs_newfs_only[i]);
		l->head = NAME_PREFIX;
		l->fmt = strdup(devdev);
		l->todo = "c";
		l->var = __UNCONST(extern_fs_newfs_only[i]);
		l->func = found_fs_nocheck;
		l++;
	}
	assert((size_t)(l - fstabbuf) == num_entries);

	/* First the root device. */
	if (target_already_root()) {
		/* avoid needing to call target_already_root() again */
		targetroot_mnt[0] = 0;
	} else if (pm->no_part) {
		snprintf(devdev, sizeof devdev, _PATH_DEV "%s", pm->diskdev);
		error = mount_root(devdev, true, false, install);
		if (error != 0 && error != EBUSY)
			return -1;
	} else {
		for (i = 0; i < install->num; i++) {
			if (is_root_part_mount(install->infos[i].mount))
				break;
		}

		if (i >= install->num) {
			hit_enter_to_continue(MSG_noroot, NULL);
			return -1;
		}

		if (!install->infos[i].parts->pscheme->get_part_device(
		    install->infos[i].parts, install->infos[i].cur_part_id,
		    devdev, sizeof devdev, NULL, plain_name, true, true))
			return -1;
		error = mount_root(devdev, true, false, install);
		if (error != 0 && error != EBUSY)
			return -1;
	}

	/* Check the target /etc/fstab exists before trying to parse it. */
	if (target_dir_exists_p("/etc") == 0 ||
	    target_file_exists_p("/etc/fstab") == 0) {
		msg_fmt_display(MSG_noetcfstab, "%s", pm->diskdev);
		hit_enter_to_continue(NULL, NULL);
		return -1;
	}


	/* Get fstab entries from the target-root /etc/fstab. */
	fstabsize = target_collect_file(T_FILE, &fstab, "/etc/fstab");
	if (fstabsize < 0) {
		/* error ! */
		msg_fmt_display(MSG_badetcfstab, "%s", pm->diskdev);
		hit_enter_to_continue(NULL, NULL);
		umount_root();
		return -2;
	}
	/*
	 * We unmount the read-only root again, so we can mount it
	 * with proper options from /etc/fstab
	 */
	umount_root();

	/*
	 * Now do all entries in /etc/fstab and mount them if required
	 */
	error = walk(fstab, (size_t)fstabsize, fstabbuf, num_entries);
	free(fstab);
	for (i = 0; i < num_entries; i++)
		free(__UNCONST(fstabbuf[i].fmt));
	free(fstabbuf);

	return error;
}

static char swap_dev[PATH_MAX];

void
set_swap_if_low_ram(struct install_partition_desc *install)
{
	swap_dev[0] = 0;
	if (get_ramsize() <= TINY_RAM_SIZE)
		set_swap(install);
}

void
set_swap(struct install_partition_desc *install)
{
	size_t i;
	int rval;

	swap_dev[0] = 0;
	for (i = 0; i < install->num; i++) {
		if (install->infos[i].type == PT_swap)
			break;
	}
	if (i >= install->num)
		return;

	if (!install->infos[i].parts->pscheme->get_part_device(
	    install->infos[i].parts, install->infos[i].cur_part_id, swap_dev,
	    sizeof swap_dev, NULL, plain_name, true, true))
		return;

	rval = swapctl(SWAP_ON, swap_dev, 0);
	if (rval != 0)
		swap_dev[0] = 0;
}

void
clear_swap(void)
{

	if (swap_dev[0] == 0)
		return;
	swapctl(SWAP_OFF, swap_dev, 0);
	swap_dev[0] = 0;
}

int
check_swap(const char *disk, int remove_swap)
{
	struct swapent *swap;
	char *cp;
	int nswap;
	int l;
	int rval = 0;

	nswap = swapctl(SWAP_NSWAP, 0, 0);
	if (nswap <= 0)
		return 0;

	swap = malloc(nswap * sizeof *swap);
	if (swap == NULL)
		return -1;

	nswap = swapctl(SWAP_STATS, swap, nswap);
	if (nswap < 0)
		goto bad_swap;

	l = strlen(disk);
	while (--nswap >= 0) {
		/* Should we check the se_dev or se_path? */
		cp = swap[nswap].se_path;
		if (memcmp(cp, "/dev/", 5) != 0)
			continue;
		if (memcmp(cp + 5, disk, l) != 0)
			continue;
		if (!isalpha(*(unsigned char *)(cp + 5 + l)))
			continue;
		if (cp[5 + l + 1] != 0)
			continue;
		/* ok path looks like it is for this device */
		if (!remove_swap) {
			/* count active swap areas */
			rval++;
			continue;
		}
		if (swapctl(SWAP_OFF, cp, 0) == -1)
			rval = -1;
	}

    done:
	free(swap);
	return rval;

    bad_swap:
	rval = -1;
	goto done;
}

#ifdef HAVE_BOOTXX_xFS
char *
bootxx_name(struct install_partition_desc *install)
{
	size_t i;
	int fstype = -1;
	const char *bootxxname;
	char *bootxx;

	/* find a partition to be mounted as / */
	for (i = 0; i < install->num; i++) {
		if ((install->infos[i].instflags & PUIINST_MOUNT)
		    && strcmp(install->infos[i].mount, "/") == 0) {
			fstype = install->infos[i].fs_type;
			break;
		}
	}
	if (fstype < 0) {
		/* not found? take first root type partition instead */
		for (i = 0; i < install->num; i++) {
			if (install->infos[i].type == PT_root) {
				fstype = install->infos[i].fs_type;
				break;
			}
		}
	}

	/* check we have boot code for the root partition type */
	switch (fstype) {
#if defined(BOOTXX_FFSV1) || defined(BOOTXX_FFSV2)
	case FS_BSDFFS:
		if (install->infos[i].fs_version == 2) {
#ifdef BOOTXX_FFSV2
			bootxxname = BOOTXX_FFSV2;
#else
			bootxxname = NULL;
#endif
		} else {
#ifdef BOOTXX_FFSV1
			bootxxname = BOOTXX_FFSV1;
#else
			bootxxname = NULL;
#endif
		}
		break;
#endif
#ifdef BOOTXX_LFSV2
	case FS_BSDLFS:
		bootxxname = BOOTXX_LFSV2;
		break;
#endif
	default:
		bootxxname = NULL;
		break;
	}

	if (bootxxname == NULL)
		return NULL;

	asprintf(&bootxx, "%s/%s", BOOTXXDIR, bootxxname);
	return bootxx;
}
#endif

/* from dkctl.c */
static int
get_dkwedges_sort(const void *a, const void *b)
{
	const struct dkwedge_info *dkwa = a, *dkwb = b;
	const daddr_t oa = dkwa->dkw_offset, ob = dkwb->dkw_offset;
	return (oa < ob) ? -1 : (oa > ob) ? 1 : 0;
}

int
get_dkwedges(struct dkwedge_info **dkw, const char *diskdev)
{
	struct dkwedge_list dkwl;

	*dkw = NULL;
	if (!get_wedge_list(diskdev, &dkwl))
		return -1;

	if (dkwl.dkwl_nwedges > 0 && *dkw != NULL) {
		qsort(*dkw, dkwl.dkwl_nwedges, sizeof(**dkw),
		    get_dkwedges_sort);
	}

	return dkwl.dkwl_nwedges;
}

#ifndef NO_CLONES
/*
 * Helper structures used in the partition select menu
 */
struct single_partition {
	struct disk_partitions *parts;
	part_id id;
};

struct sel_menu_data {
	struct single_partition *partitions;
	struct selected_partition result;
};

static int
select_single_part(menudesc *m, void *arg)
{
	struct sel_menu_data *data = arg;

	data->result.parts = data->partitions[m->cursel].parts;
	data->result.id = data->partitions[m->cursel].id;

	return 1;
}

static void
display_single_part(menudesc *m, int opt, void *arg)
{
	const struct sel_menu_data *data = arg;
	struct disk_part_info info;
	struct disk_partitions *parts = data->partitions[opt].parts;
	part_id id = data->partitions[opt].id;
	int l;
	const char *desc = NULL;
	char line[MENUSTRSIZE*2];

	if (!parts->pscheme->get_part_info(parts, id, &info))
		return;

	if (parts->pscheme->other_partition_identifier != NULL)
		desc = parts->pscheme->other_partition_identifier(
		    parts, id);

	daddr_t start = info.start / sizemult;
	daddr_t size = info.size / sizemult;
	snprintf(line, sizeof line, "%s [%" PRIu64 " @ %" PRIu64 "]",
	    parts->disk, size, start);

	if (info.nat_type != NULL) {
		strlcat(line, " ", sizeof line);
		strlcat(line, info.nat_type->description, sizeof line);
	}

	if (desc != NULL) {
		strlcat(line, ": ", sizeof line);
		strlcat(line, desc, sizeof line);
	}

	l = strlen(line);
	if (l >= (m->w))
		strcpy(line + (m->w-3), "...");
	wprintw(m->mw, "%s", line);
}

/*
 * is the given "test" partitions set used in the selected set?
 */
static bool
selection_has_parts(struct selected_partitions *sel,
    const struct disk_partitions *test)
{
	size_t i;

	for (i = 0; i < sel->num_sel; i++) {
		if (sel->selection[i].parts == test)
			return true;
	}
	return false;
}

/*
 * is the given "test" partition in the selected set?
 */
static bool
selection_has_partition(struct selected_partitions *sel,
    const struct disk_partitions *test, part_id test_id)
{
	size_t i;

	for (i = 0; i < sel->num_sel; i++) {
		if (sel->selection[i].parts == test &&
		    sel->selection[i].id == test_id)
			return true;
	}
	return false;
}

/*
 * let the user select a partition, optionally skipping all partitions
 * on the "ignore" device
 */
static bool
add_select_partition(struct selected_partitions *res,
    struct disk_partitions **all_parts, size_t all_cnt)
{
	struct disk_partitions *ps;
	struct disk_part_info info;
	part_id id;
	struct single_partition *partitions, *pp;
	struct menu_ent *part_menu_opts, *menup;
	size_t n, part_cnt;
	int sel_menu;

	/*
	 * count how many items our menu will have
	 */
	part_cnt = 0;
	for (n = 0; n < all_cnt; n++) {
		ps = all_parts[n];
		for (id = 0; id < ps->num_part; id++) {
			if (selection_has_partition(res, ps, id))
				continue;
			if (!ps->pscheme->get_part_info(ps, id, &info))
				continue;
			if (info.flags & (PTI_SEC_CONTAINER|PTI_WHOLE_DISK|
			    PTI_PSCHEME_INTERNAL|PTI_RAW_PART))
				continue;
			part_cnt++;
		}
	}

	/*
	 * create a menu from this and let the user
	 * select one partition
	 */
	part_menu_opts = NULL;
	partitions = calloc(part_cnt, sizeof *partitions);
	if (partitions == NULL)
		goto done;
	part_menu_opts = calloc(part_cnt, sizeof *part_menu_opts);
	if (part_menu_opts == NULL)
		goto done;
	pp = partitions;
	menup = part_menu_opts;
	for (n = 0; n < all_cnt; n++) {
		ps = all_parts[n];
		for (id = 0; id < ps->num_part; id++) {
			if (selection_has_partition(res, ps, id))
				continue;
			if (!ps->pscheme->get_part_info(ps, id, &info))
				continue;
			if (info.flags & (PTI_SEC_CONTAINER|PTI_WHOLE_DISK|
			    PTI_PSCHEME_INTERNAL|PTI_RAW_PART))
				continue;
			pp->parts = ps;
			pp->id = id;
			pp++;
			menup->opt_action = select_single_part;
			menup++;
		}
	}
	sel_menu = new_menu(MSG_select_foreign_part, part_menu_opts, part_cnt,
	    3, 3, 0, 60,
	    MC_SUBMENU | MC_SCROLL | MC_NOCLEAR,
	    NULL, display_single_part, NULL,
	    NULL, MSG_exit_menu_generic);
	if (sel_menu != -1) {
		struct selected_partition *newsels;
		struct sel_menu_data data;

		memset(&data, 0, sizeof data);
		data.partitions = partitions;
		process_menu(sel_menu, &data);
		free_menu(sel_menu);

		if (data.result.parts != NULL) {
			newsels = realloc(res->selection,
			    sizeof(*res->selection)*(res->num_sel+1));
			if (newsels != NULL) {
				res->selection = newsels;
				newsels += res->num_sel++;
				newsels->parts = data.result.parts;
				newsels->id = data.result.id;
			}
		}
	}

	/*
	 * Final cleanup
	 */
done:
	free(part_menu_opts);
	free(partitions);

	return res->num_sel > 0;
}

struct part_selection_and_all_parts {
	struct selected_partitions *selection;
	struct disk_partitions **all_parts;
	size_t all_cnt;
	char *title;
	bool cancelled;
};

static int
toggle_clone_data(struct menudesc *m, void *arg)
{
	struct part_selection_and_all_parts *sel = arg;

	sel->selection->with_data = !sel->selection->with_data;
	return 0;
}

static int
add_another(struct menudesc *m, void *arg)
{
	struct part_selection_and_all_parts *sel = arg;

	add_select_partition(sel->selection, sel->all_parts, sel->all_cnt);
	return 0;
}

static int
cancel_clone(struct menudesc *m, void *arg)
{
	struct part_selection_and_all_parts *sel = arg;

	sel->cancelled = true;
	return 1;
}

static void
update_sel_part_title(struct part_selection_and_all_parts *sel)
{
	struct disk_part_info info;
	char *buf, line[MENUSTRSIZE];
	size_t buf_len, i;

	buf_len = MENUSTRSIZE * (1+sel->selection->num_sel);
	buf = malloc(buf_len);
	if (buf == NULL)
		return;

	strcpy(buf, msg_string(MSG_select_source_hdr));
	for (i = 0; i < sel->selection->num_sel; i++) {
		struct selected_partition *s =
		    &sel->selection->selection[i];
		if (!s->parts->pscheme->get_part_info(s->parts, s->id, &info))
			continue;
		daddr_t start = info.start / sizemult;
		daddr_t size = info.size / sizemult;
		sprintf(line, "\n  %s [%" PRIu64 " @ %" PRIu64 "] ",
		    s->parts->disk, size, start);
		if (info.nat_type != NULL)
			strlcat(line, info.nat_type->description, sizeof(line));
		strlcat(buf, line, buf_len);
	}
	free(sel->title);
	sel->title = buf;
}

static void
post_sel_part(struct menudesc *m, void *arg)
{
	struct part_selection_and_all_parts *sel = arg;

	if (m->mw == NULL)
		return;
	update_sel_part_title(sel);
	m->title = sel->title;
	m->h = 0;
	resize_menu_height(m);
}

static void
fmt_sel_part_line(struct menudesc *m, int i, void *arg)
{
	struct part_selection_and_all_parts *sel = arg;

	wprintw(m->mw, "%s: %s", msg_string(MSG_clone_with_data),
	    sel->selection->with_data ?
		msg_string(MSG_Yes) :
		 msg_string(MSG_No));
}

bool
select_partitions(struct selected_partitions *res,
    const struct disk_partitions *ignore)
{
	struct disk_desc disks[MAX_DISKS];
	struct disk_partitions *ps;
	struct part_selection_and_all_parts data;
	struct pm_devs *i;
	size_t j;
	int cnt, n, m;
	static menu_ent men[] = {
		{ .opt_name = MSG_select_source_add,
		  .opt_action = add_another },
		{ .opt_action = toggle_clone_data },
		{ .opt_name = MSG_cancel, .opt_action = cancel_clone },
	};

	memset(res, 0, sizeof *res);
	memset(&data, 0, sizeof data);
	data.selection = res;

	/*
	 * collect all available partition sets
	 */
	data.all_cnt = 0;
	if (SLIST_EMPTY(&pm_head)) {
		cnt = get_disks(disks, false);
		if (cnt <= 0)
			return false;

		/*
		 * allocate two slots for each disk (primary/secondary)
		 */
		data.all_parts = calloc(2*cnt, sizeof *data.all_parts);
		if (data.all_parts == NULL)
			return false;

		for (n = 0; n < cnt; n++) {
			if (ignore != NULL &&
			    strcmp(disks[n].dd_name, ignore->disk) == 0)
				continue;

			ps = partitions_read_disk(disks[n].dd_name,
			    disks[n].dd_totsec,
			    disks[n].dd_secsize,
			    disks[n].dd_no_mbr);
			if (ps == NULL)
				continue;
			data.all_parts[data.all_cnt++] = ps;
			ps = get_inner_parts(ps);
			if (ps == NULL)
				continue;
			data.all_parts[data.all_cnt++] = ps;
		}
		if (data.all_cnt > 0)
			res->free_parts = true;
	} else {
		cnt = 0;
		SLIST_FOREACH(i, &pm_head, l)
			cnt++;

		data.all_parts = calloc(cnt, sizeof *data.all_parts);
		if (data.all_parts == NULL)
			return false;

		SLIST_FOREACH(i, &pm_head, l) {
			if (i->parts == NULL)
				continue;
			if (i->parts == ignore)
				continue;
			data.all_parts[data.all_cnt++] = i->parts;
		}
	}

	if (!add_select_partition(res, data.all_parts, data.all_cnt))
		goto fail;

	/* loop with menu */
	update_sel_part_title(&data);
	m = new_menu(data.title, men, __arraycount(men), 3, 2, 0, 65, MC_SCROLL,
	    post_sel_part, fmt_sel_part_line, NULL, NULL, MSG_clone_src_done);
	process_menu(m, &data);
	free(data.title);
	if (res->num_sel == 0)
		goto fail;

	/* cleanup */
	if (res->free_parts) {
		for (j = 0; j < data.all_cnt; j++) {
			if (selection_has_parts(res, data.all_parts[j]))
				continue;
			if (data.all_parts[j]->parent != NULL)
				continue;
			data.all_parts[j]->pscheme->free(data.all_parts[j]);
		}
	}
	free(data.all_parts);
	return true;

fail:
	if (res->free_parts) {
		for (j = 0; j < data.all_cnt; j++) {
			if (data.all_parts[j]->parent != NULL)
				continue;
			data.all_parts[j]->pscheme->free(data.all_parts[j]);
		}
	}
	free(data.all_parts);
	return false;
}

void
free_selected_partitions(struct selected_partitions *selected)
{
	size_t i;
	struct disk_partitions *parts;

	if (!selected->free_parts)
		return;

	for (i = 0; i < selected->num_sel; i++) {
		parts = selected->selection[i].parts;

		/* remove from list before testing for other instances */
		selected->selection[i].parts = NULL;

		/* if this is the secondary partition set, the parent owns it */
		if (parts->parent != NULL)
			continue;

		/* only free once (we use the last one) */
		if (selection_has_parts(selected, parts))
			continue;
		parts->pscheme->free(parts);
	}
	free(selected->selection);
}

daddr_t
selected_parts_size(struct selected_partitions *selected)
{
	struct disk_part_info info;
	size_t i;
	daddr_t s = 0;

	for (i = 0; i < selected->num_sel; i++) {
		if (!selected->selection[i].parts->pscheme->get_part_info(
		    selected->selection[i].parts,
		    selected->selection[i].id, &info))
			continue;
		s += info.size;
	}

	return s;
}

int
clone_target_select(menudesc *m, void *arg)
{
	struct clone_target_menu_data *data = arg;

	data->res = m->cursel;
	return 1;
}

bool
clone_partition_data(struct disk_partitions *dest_parts, part_id did,
    struct disk_partitions *src_parts, part_id sid)
{
	char src_dev[MAXPATHLEN], target_dev[MAXPATHLEN];

	if (!src_parts->pscheme->get_part_device(
	    src_parts, sid, src_dev, sizeof src_dev, NULL,
	    raw_dev_name, true, true))
		return false;
	if (!dest_parts->pscheme->get_part_device(
	    dest_parts, did, target_dev, sizeof target_dev, NULL,
	    raw_dev_name, true, true))
		return false;

	return run_program(RUN_DISPLAY | RUN_PROGRESS,
	    "progress -f %s -b 1m dd bs=1m of=%s",
	    src_dev, target_dev) == 0;
}
#endif


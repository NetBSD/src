/*	$NetBSD: disks.c,v 1.131 2014/07/12 16:11:27 dholland Exp $ */

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


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <util.h>

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/swap.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#define FSTYPENAMES
#include <sys/disklabel.h>

#include <dev/scsipi/scsipi_all.h>
#include <sys/scsiio.h>

#include <dev/ata/atareg.h>
#include <sys/ataio.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"
#include "txtwalk.h"

/* Disk descriptions */
#define MAX_DISKS 15
struct disk_desc {
	char	dd_name[SSTRSIZE];
	char	dd_descr[70];
	uint	dd_no_mbr;
	uint	dd_cyl;
	uint	dd_head;
	uint	dd_sec;
	uint	dd_secsize;
	uint	dd_totsec;
};

/* Local prototypes */
static int foundffs(struct data *, size_t);
#ifdef USE_SYSVBFS
static int foundsysvbfs(struct data *, size_t);
#endif
static int fsck_preen(const char *, int, const char *);
static void fixsb(const char *, const char *, char);

#ifndef DISK_NAMES
#define DISK_NAMES "wd", "sd", "ld", "raid"
#endif

static const char *disk_names[] = { DISK_NAMES, "vnd", NULL };

static bool tmpfs_on_var_shm(void);

const char *
getfslabelname(uint8_t f)
{
	if (f >= __arraycount(fstypenames) || fstypenames[f] == NULL)
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

	return ram > 16UL*1024UL*1024UL;
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
get_descr_scsi(struct disk_desc *dd, int fd)
{
	struct scsipi_inquiry_data inqbuf;
	struct scsipi_inquiry cmd;
	scsireq_t req;
        /* x4 in case every character is escaped, +1 for NUL. */
	char vendor[(sizeof(inqbuf.vendor) * 4) + 1],
	     product[(sizeof(inqbuf.product) * 4) + 1],
	     revision[(sizeof(inqbuf.revision) * 4) + 1];
	char size[5];
	int error;

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

	error = ioctl(fd, SCIOCCOMMAND, &req);
	if (error == -1 || req.retsts != SCCMD_OK)
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
get_descr_ata(struct disk_desc *dd, int fd)
{
	struct atareq req;
	static union {
		unsigned char inbuf[DEV_BSIZE];
		struct ataparams inqbuf;
	} inbuf;
	struct ataparams *inqbuf = &inbuf.inqbuf;
	char model[sizeof(inqbuf->atap_model)+1];
	char size[5];
	int error, needswap = 0;

	memset(&inbuf, 0, sizeof(inbuf));
	memset(&req, 0, sizeof(req));

	req.flags = ATACMD_READ;
	req.command = WDCC_IDENTIFY;
	req.databuf = (void *)&inbuf;
	req.datalen = sizeof(inbuf);
	req.timeout = 1000;

	error = ioctl(fd, ATAIOCCOMMAND, &req);
	if (error == -1 || req.retsts != ATACMD_OK)
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

static void
get_descr(struct disk_desc *dd)
{
	char diskpath[MAXPATHLEN];
	int fd = -1;

	fd = opendisk(dd->dd_name, O_RDONLY, diskpath, sizeof(diskpath), 0);
	if (fd < 0)
		goto done;

	dd->dd_descr[0] = '\0';

	/* try ATA */
	if (get_descr_ata(dd, fd))
		goto done;
	/* try SCSI */
	if (get_descr_scsi(dd, fd))
		goto done;

done:
	if (fd >= 0)
		close(fd);
	if (strlen(dd->dd_descr) == 0)
		strcpy(dd->dd_descr, dd->dd_name);
}

/* disknames - contains device names without partition letters
 * cdrom_devices - contains devices including partition letters
 * returns the first entry in hw.disknames matching a cdrom_device, or
 * first entry on error or no match
 */
const char *
get_default_cdrom(void)
{
	static const char *cdrom_devices[] = { CD_NAMES, 0};
	static const char mib_name[] = "hw.disknames";
	size_t len;
	char *disknames;
	char *last;
	char *name;
	const char **arg;
	const char *cd_dev;

	/* On error just use first entry in cdrom_devices */
	if (sysctlbyname(mib_name, NULL, &len, NULL, 0) == -1)
		return cdrom_devices[0];
	if ((disknames = malloc(len + 2)) == 0) /* skip on malloc fail */
		return cdrom_devices[0];

	(void)sysctlbyname(mib_name, disknames, &len, NULL, 0);
        for ((name = strtok_r(disknames, " ", &last)); name;
	    (name = strtok_r(NULL, " ", &last))) {
		for (arg = cdrom_devices; *arg; ++arg) {
			cd_dev = *arg;
			/* skip unit and partition */
			if (strncmp(cd_dev, name, strlen(cd_dev) - 2) != 0)
				continue;
			if (name != disknames)
				strcpy(disknames, name);
			strcat(disknames, "a");
			/* XXX: leaks, but so what? */
			return disknames;
		}
	}
	free(disknames);
	return cdrom_devices[0];
}

static int
get_disks(struct disk_desc *dd)
{
	const char **xd;
	char *cp;
	struct disklabel l;
	int i;
	int numdisks;

	/* initialize */
	numdisks = 0;

	for (xd = disk_names; *xd != NULL; xd++) {
		for (i = 0; i < MAX_DISKS; i++) {
			strlcpy(dd->dd_name, *xd, sizeof dd->dd_name - 2);
			cp = strchr(dd->dd_name, ':');
			if (cp != NULL)
				dd->dd_no_mbr = !strcmp(cp, ":no_mbr");
			else {
				dd->dd_no_mbr = 0;
				cp = strchr(dd->dd_name, 0);
			}

			snprintf(cp, 2 + 1, "%d", i);
			if (!get_geom(dd->dd_name, &l)) {
				if (errno == ENOENT)
					break;
				continue;
			}

			/*
			 * Exclude a disk mounted as root partition,
			 * in case of install-image on a USB memstick.
			 */
			if (is_active_rootpart(dd->dd_name, 0))
				continue;

			dd->dd_cyl = l.d_ncylinders;
			dd->dd_head = l.d_ntracks;
			dd->dd_sec = l.d_nsectors;
			dd->dd_secsize = l.d_secsize;
			dd->dd_totsec = l.d_secperunit;
			get_descr(dd);
			dd++;
			numdisks++;
			if (numdisks >= MAX_DISKS)
				return numdisks;
		}
	}
	return numdisks;
}

static int
set_dsk_select(menudesc *m, void *arg)
{
	*(int *)arg = m->cursel;
	return 1;
}

int
find_disks(const char *doingwhat)
{
	struct disk_desc disks[MAX_DISKS];
	menu_ent dsk_menu[nelem(disks)];
	struct disk_desc *disk;
	int i;
	int numdisks;
	int selected_disk = 0;
	int menu_no;

	/* Find disks. */
	numdisks = get_disks(disks);

	/* need a redraw here, kernel messages hose everything */
	touchwin(stdscr);
	refresh();
	/* Kill typeahead, it won't be what the user had in mind */
	fpurge(stdin);

	if (numdisks == 0) {
		/* No disks found! */
		msg_display(MSG_nodisk);
		process_menu(MENU_ok, NULL);
		/*endwin();*/
		return -1;
	}

	if (numdisks == 1) {
		/* One disk found! */
		msg_display(MSG_onedisk, disks[0].dd_descr, doingwhat);
		process_menu(MENU_ok, NULL);
	} else {
		/* Multiple disks found! */
		for (i = 0; i < numdisks; i++) {
			dsk_menu[i].opt_name = disks[i].dd_descr;
			dsk_menu[i].opt_menu = OPT_NOMENU;
			dsk_menu[i].opt_flags = OPT_EXIT;
			dsk_menu[i].opt_action = set_dsk_select;
		}
		menu_no = new_menu(MSG_Available_disks,
			dsk_menu, numdisks, -1, 4, 0, 0,
			MC_SCROLL | MC_NOEXITOPT,
			NULL, NULL, NULL, NULL, NULL);
		if (menu_no == -1)
			return -1;
		msg_display(MSG_ask_disk, doingwhat);
		process_menu(menu_no, &selected_disk);
		free_menu(menu_no);
	}

	disk = disks + selected_disk;
	strlcpy(diskdev, disk->dd_name, sizeof diskdev);

	/* Use as a default disk if the user has the sets on a local disk */
	strlcpy(localfs_dev, disk->dd_name, sizeof localfs_dev);

	sectorsize = disk->dd_secsize;
	dlcyl = disk->dd_cyl;
	dlhead = disk->dd_head;
	dlsec = disk->dd_sec;
	dlsize = disk->dd_totsec;
	no_mbr = disk->dd_no_mbr;
	if (dlsize == 0)
		dlsize = disk->dd_cyl * disk->dd_head * disk->dd_sec;
	if (dlsize > UINT32_MAX) {
		msg_display(MSG_toobigdisklabel);
		process_menu(MENU_ok, NULL);
		return -1;
	}
	dlcylsize = dlhead * dlsec;

	/* Get existing/default label */
	memset(&oldlabel, 0, sizeof oldlabel);
	incorelabel(diskdev, oldlabel);

	/* Set 'target' label to current label in case we don't change it */
	memcpy(&bsdlabel, &oldlabel, sizeof bsdlabel);

	return numdisks;
}

void
fmt_fspart(menudesc *m, int ptn, void *arg)
{
	unsigned int poffset, psize, pend;
	const char *desc;
	static const char *Yes;
	partinfo *p = bsdlabel + ptn;

	if (Yes == NULL)
		Yes = msg_string(MSG_Yes);

	poffset = p->pi_offset / sizemult;
	psize = p->pi_size / sizemult;
	if (psize == 0)
		pend = 0;
	else
		pend = (p->pi_offset + p->pi_size) / sizemult - 1;

	if (p->pi_fstype == FS_BSDFFS)
		if (p->pi_flags & PIF_FFSv2)
			desc = "FFSv2";
		else
			desc = "FFSv1";
	else
		desc = getfslabelname(p->pi_fstype);

#ifdef PART_BOOT
	if (ptn == PART_BOOT)
		desc = msg_string(MSG_Boot_partition_cant_change);
#endif
	if (ptn == getrawpartition())
		desc = msg_string(MSG_Whole_disk_cant_change);
	else {
		if (ptn == PART_C)
			desc = msg_string(MSG_NetBSD_partition_cant_change);
	}

	wprintw(m->mw, msg_string(MSG_fspart_row),
			poffset, pend, psize, desc,
			p->pi_flags & PIF_NEWFS ? Yes : "",
			p->pi_flags & PIF_MOUNT ? Yes : "",
			p->pi_mount);
}

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
int
write_disklabel (void)
{

#ifdef DISKLABEL_CMD
	/* disklabel the disk */
	return run_program(RUN_DISPLAY, "%s -f /tmp/disktab %s '%s'",
	    DISKLABEL_CMD, diskdev, bsddiskname);
#else
	return 0;
#endif
}


static int
ptn_sort(const void *a, const void *b)
{
	return strcmp(bsdlabel[*(const int *)a].pi_mount,
		      bsdlabel[*(const int *)b].pi_mount);
}

int
make_filesystems(void)
{
	unsigned int i;
	int ptn;
	int ptn_order[nelem(bsdlabel)];
	int error = 0;
	unsigned int maxpart = getmaxpartitions();
	char *newfs;
	const char *mnt_opts;
	const char *fsname;
	partinfo *lbl;

	if (maxpart > nelem(bsdlabel))
		maxpart = nelem(bsdlabel);

	/* Making new file systems and mounting them */

	/* sort to ensure /usr/local is mounted after /usr (etc) */
	for (i = 0; i < maxpart; i++)
		ptn_order[i] = i;
	qsort(ptn_order, maxpart, sizeof ptn_order[0], ptn_sort);

	for (i = 0; i < maxpart; i++) {
		/*
		 * newfs and mount. For now, process only BSD filesystems.
		 * but if this is the mounted-on root, has no mount
		 * point defined, or is marked preserve, don't touch it!
		 */
		ptn = ptn_order[i];
		lbl = bsdlabel + ptn;

		if (is_active_rootpart(diskdev, ptn))
			continue;

		if (*lbl->pi_mount == 0)
			/* No mount point */
			continue;

		newfs = NULL;
		mnt_opts = NULL;
		fsname = NULL;
		switch (lbl->pi_fstype) {
		case FS_APPLEUFS:
			asprintf(&newfs, "/sbin/newfs %s%.0d",
				lbl->pi_isize != 0 ? "-i" : "", lbl->pi_isize);
			mnt_opts = "-tffs -o async";
			fsname = "ffs";
			break;
		case FS_BSDFFS:
			asprintf(&newfs,
			    "/sbin/newfs -V2 -O %d -b %d -f %d%s%.0d",
			    lbl->pi_flags & PIF_FFSv2 ? 2 : 1,
			    lbl->pi_fsize * lbl->pi_frag, lbl->pi_fsize,
			    lbl->pi_isize != 0 ? " -i " : "", lbl->pi_isize);
			if (lbl->pi_flags & PIF_LOG)
				mnt_opts = "-tffs -o log";
			else
				mnt_opts = "-tffs -o async";
			fsname = "ffs";
			break;
		case FS_BSDLFS:
			asprintf(&newfs, "/sbin/newfs_lfs -b %d",
				lbl->pi_fsize * lbl->pi_frag);
			mnt_opts = "-tlfs";
			fsname = "lfs";
			break;
		case FS_MSDOS:
#ifdef USE_NEWFS_MSDOS
			asprintf(&newfs, "/sbin/newfs_msdos");
#endif
			mnt_opts = "-tmsdos";
			fsname = "msdos";
			break;
#ifdef USE_SYSVBFS
		case FS_SYSVBFS:
			asprintf(&newfs, "/sbin/newfs_sysvbfs");
			mnt_opts = "-tsysvbfs";
			fsname = "sysvbfs";
			break;
#endif
#ifdef USE_EXT2FS
		case FS_EX2FS:
			asprintf(&newfs, "/sbin/newfs_ext2fs");
			mnt_opts = "-text2fs";
			fsname = "ext2fs";
			break;
#endif
		}
		if (lbl->pi_flags & PIF_NEWFS && newfs != NULL) {
#ifdef USE_NEWFS_MSDOS
			if (lbl->pi_fstype == FS_MSDOS) {
			        /* newfs only if mount fails */
			        if (run_program(RUN_SILENT | RUN_ERROR_OK,
				    "mount -rt msdos /dev/%s%c /mnt2",
				    diskdev, 'a' + ptn) != 0)
					error = run_program(
					    RUN_DISPLAY | RUN_PROGRESS,
					    "%s /dev/r%s%c",
					    newfs, diskdev, 'a' + ptn);
				else {
			        	run_program(RUN_SILENT | RUN_ERROR_OK,
					    "umount /mnt2");
					error = 0;
				}
			} else
#endif
			error = run_program(RUN_DISPLAY | RUN_PROGRESS,
			    "%s /dev/r%s%c", newfs, diskdev, 'a' + ptn);
		} else {
			/* We'd better check it isn't dirty */
			error = fsck_preen(diskdev, ptn, fsname);
		}
		free(newfs);
		if (error != 0)
			return error;

		md_pre_mount();

		if (lbl->pi_flags & PIF_MOUNT && mnt_opts != NULL) {
			make_target_dir(lbl->pi_mount);
			error = target_mount(mnt_opts, diskdev, ptn,
					    lbl->pi_mount);
			if (error) {
				msg_display(MSG_mountfail,
					    diskdev, 'a' + ptn, lbl->pi_mount);
				process_menu(MENU_ok, NULL);
				return error;
			}
		}
	}
	return 0;
}

int
make_fstab(void)
{
	FILE *f;
	int i, swap_dev = -1;
	const char *dump_dev;

	/* Create the fstab. */
	make_target_dir("/etc");
	f = target_fopen("/etc/fstab", "w");
	if (logfp)
		(void)fprintf(logfp,
		    "Creating %s/etc/fstab.\n", target_prefix());
	scripting_fprintf(NULL, "cat <<EOF >%s/etc/fstab\n", target_prefix());

	if (f == NULL) {
#ifndef DEBUG
		msg_display(MSG_createfstab);
		if (logfp)
			(void)fprintf(logfp, "Failed to make /etc/fstab!\n");
		process_menu(MENU_ok, NULL);
		return 1;
#else
		f = stdout;
#endif
	}

	scripting_fprintf(f, "# NetBSD %s/etc/fstab\n# See /usr/share/examples/"
		"fstab/ for more examples.\n", target_prefix());
	for (i = 0; i < getmaxpartitions(); i++) {
		const char *s = "";
		const char *mp = bsdlabel[i].pi_mount;
		const char *fstype = "ffs";
		int fsck_pass = 0, dump_freq = 0;

		if (!*mp) {
			/*
			 * No mount point specified, comment out line and
			 * use /mnt as a placeholder for the mount point.
			 */
			s = "# ";
			mp = "/mnt";
		}

		switch (bsdlabel[i].pi_fstype) {
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
			if (swap_dev == -1) {
				swap_dev = i;
				dump_dev = ",dp";
			} else {
				dump_dev ="";
			}
			scripting_fprintf(f, "/dev/%s%c\t\tnone\tswap\tsw%s\t\t 0 0\n",
				diskdev, 'a' + i, dump_dev);
			continue;
#ifdef USE_SYSVBFS
		case FS_SYSVBFS:
			fstype = "sysvbfs";
			make_target_dir("/stand");
			break;
#endif
		default:
			fstype = "???";
			s = "# ";
			break;
		}
		/* The code that remounts root rw doesn't check the partition */
		if (strcmp(mp, "/") == 0 && !(bsdlabel[i].pi_flags & PIF_MOUNT))
			s = "# ";

 		scripting_fprintf(f,
		  "%s/dev/%s%c\t\t%s\t%s\trw%s%s%s%s%s%s%s%s\t\t %d %d\n",
		   s, diskdev, 'a' + i, mp, fstype,
		   bsdlabel[i].pi_flags & PIF_LOG ? ",log" : "",
		   bsdlabel[i].pi_flags & PIF_MOUNT ? "" : ",noauto",
		   bsdlabel[i].pi_flags & PIF_ASYNC ? ",async" : "",
		   bsdlabel[i].pi_flags & PIF_NOATIME ? ",noatime" : "",
		   bsdlabel[i].pi_flags & PIF_NODEV ? ",nodev" : "",
		   bsdlabel[i].pi_flags & PIF_NODEVMTIME ? ",nodevmtime" : "",
		   bsdlabel[i].pi_flags & PIF_NOEXEC ? ",noexec" : "",
		   bsdlabel[i].pi_flags & PIF_NOSUID ? ",nosuid" : "",
		   dump_freq, fsck_pass);
	}

	if (tmp_ramdisk_size != 0) {
#ifdef HAVE_TMPFS
		scripting_fprintf(f, "tmpfs\t\t/tmp\ttmpfs\trw,-m=1777,-s=%"
		    PRIi64 "\n",
		    tmp_ramdisk_size * 512);
#else
		if (swap_dev != -1)
			scripting_fprintf(f, "/dev/%s%c\t\t/tmp\tmfs\trw,-s=%"
			    PRIi64 "\n",
			    diskdev, 'a' + swap_dev, tmp_ramdisk_size);
		else
			scripting_fprintf(f, "swap\t\t/tmp\tmfs\trw,-s=%"
			    PRIi64 "\n",
			    tmp_ramdisk_size);
#endif
	}

	/* Add /kern, /proc and /dev/pts to fstab and make mountpoint. */
	scripting_fprintf(f, "kernfs\t\t/kern\tkernfs\trw\n");
	scripting_fprintf(f, "ptyfs\t\t/dev/pts\tptyfs\trw\n");
	scripting_fprintf(f, "procfs\t\t/proc\tprocfs\trw\n");
	scripting_fprintf(f, "/dev/%s\t\t/cdrom\tcd9660\tro,noauto\n",
	    get_default_cdrom());
	scripting_fprintf(f, "%stmpfs\t\t/var/shm\ttmpfs\trw,-m1777,-sram%%25\n",
	    tmpfs_on_var_shm() ? "" : "#");
	make_target_dir("/kern");
	make_target_dir("/proc");
	make_target_dir("/dev/pts");
	make_target_dir("/cdrom");
	make_target_dir("/var/shm");

	scripting_fprintf(NULL, "EOF\n");

#ifndef DEBUG
	fclose(f);
	fflush(NULL);
#endif
	return 0;
}



static int
/*ARGSUSED*/
foundffs(struct data *list, size_t num)
{
	int error;

	if (num < 2 || strcmp(list[1].u.s_val, "/") == 0 ||
	    strstr(list[2].u.s_val, "noauto") != NULL)
		return 0;

	error = fsck_preen(list[0].u.s_val, ' '-'a', "ffs");
	if (error != 0)
		return error;

	error = target_mount("", list[0].u.s_val, ' '-'a', list[1].u.s_val);
	if (error != 0) {
		msg_display(MSG_mount_failed, list[0].u.s_val);
		process_menu(MENU_noyes, NULL);
		if (!yesno)
			return error;
	}
	return 0;
}

#ifdef USE_SYSVBFS
static int
/*ARGSUSED*/
foundsysvbfs(struct data *list, size_t num)
{
	int error;

	if (num < 2 || strcmp(list[1].u.s_val, "/") == 0 ||
	    strstr(list[2].u.s_val, "noauto") != NULL)
		return 0;

	error = target_mount("", list[0].u.s_val, ' '-'a', list[1].u.s_val);
	if (error != 0)
		return error;
	return 0;
}
#endif

/*
 * Do an fsck. On failure, inform the user by showing a warning
 * message and doing menu_ok() before proceeding.
 * Returns 0 on success, or nonzero return code from fsck() on failure.
 */
static int
fsck_preen(const char *disk, int ptn, const char *fsname)
{
	char *prog;
	int error;

	ptn += 'a';
	if (fsname == NULL)
		return 0;
	/* first, check if fsck program exists, if not, assume ok */
	asprintf(&prog, "/sbin/fsck_%s", fsname);
	if (prog == NULL)
		return 0;
	if (access(prog, X_OK) != 0)
		return 0;
	if (!strcmp(fsname,"ffs"))
		fixsb(prog, disk, ptn);
	error = run_program(0, "%s -p -q /dev/r%s%c", prog, disk, ptn);
	free(prog);
	if (error != 0) {
		msg_display(MSG_badfs, disk, ptn, error);
		process_menu(MENU_noyes, NULL);
		if (yesno)
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
fixsb(const char *prog, const char *disk, char ptn)
{
	int fd;
	int rval;
	union {
		struct fs fs;
		char buf[SBLOCKSIZE];
	} sblk;
	struct fs *fs = &sblk.fs;

	snprintf(sblk.buf, sizeof(sblk.buf), "/dev/r%s%c",
		disk, ptn == ' ' ? 0 : ptn);
	fd = open(sblk.buf, O_RDONLY);
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
	    "%s -p -b 16 -c 4 /dev/r%s%c", prog, disk, ptn);
	/* Then downgrade to fslevel 3 */
	run_program(RUN_DISPLAY | RUN_PROGRESS,
	    "%s -p -c 3 /dev/r%s%c", prog, disk, ptn);
}

/*
 * fsck and mount the root partition.
 */
static int
mount_root(void)
{
	int	error;

	error = fsck_preen(diskdev, rootpart, "ffs");
	if (error != 0)
		return error;

	md_pre_mount();

	/* Mount /dev/<diskdev>a on target's "".
	 * If we pass "" as mount-on, Prefixing will DTRT.
	 * for now, use no options.
	 * XXX consider -o remount in case target root is
	 * current root, still readonly from single-user?
	 */
	return target_mount("", diskdev, rootpart, "");
}

/* Get information on the file systems mounted from the root filesystem.
 * Offer to convert them into 4.4BSD inodes if they are not 4.4BSD
 * inodes.  Fsck them.  Mount them.
 */

int
mount_disks(void)
{
	char *fstab;
	int   fstabsize;
	int   error;

	static struct lookfor fstabbuf[] = {
		{"/dev/", "/dev/%s %s ffs %s", "c", NULL, 0, 0, foundffs},
		{"/dev/", "/dev/%s %s ufs %s", "c", NULL, 0, 0, foundffs},
#ifdef USE_SYSVBFS
		{"/dev/", "/dev/%s %s sysvbfs %s", "c", NULL, 0, 0,
		    foundsysvbfs},
#endif
	};
	static size_t numfstabbuf = sizeof(fstabbuf) / sizeof(struct lookfor);

	/* First the root device. */
	if (target_already_root())
		/* avoid needing to call target_already_root() again */
		targetroot_mnt[0] = 0;
	else {
		error = mount_root();
		if (error != 0 && error != EBUSY)
			return -1;
	}

	/* Check the target /etc/fstab exists before trying to parse it. */
	if (target_dir_exists_p("/etc") == 0 ||
	    target_file_exists_p("/etc/fstab") == 0) {
		msg_display(MSG_noetcfstab, diskdev);
		process_menu(MENU_ok, NULL);
		return -1;
	}


	/* Get fstab entries from the target-root /etc/fstab. */
	fstabsize = target_collect_file(T_FILE, &fstab, "/etc/fstab");
	if (fstabsize < 0) {
		/* error ! */
		msg_display(MSG_badetcfstab, diskdev);
		process_menu(MENU_ok, NULL);
		return -1;
	}
	error = walk(fstab, (size_t)fstabsize, fstabbuf, numfstabbuf);
	free(fstab);

	return error;
}

int
set_swap(const char *disk, partinfo *pp)
{
	int i;
	char *cp;
	int rval;

	if (pp == NULL)
		pp = oldlabel;

	for (i = 0; i < MAXPARTITIONS; i++) {
		if (pp[i].pi_fstype != FS_SWAP)
			continue;
		asprintf(&cp, "/dev/%s%c", disk, 'a' + i);
		rval = swapctl(SWAP_ON, cp, 0);
		free(cp);
		if (rval != 0)
			return -1;
	}

	return 0;
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
bootxx_name(void)
{
	int fstype;
	const char *bootxxname;
	char *bootxx;

	/* check we have boot code for the root partition type */
	fstype = bsdlabel[rootpart].pi_fstype;
	switch (fstype) {
#if defined(BOOTXX_FFSV1) || defined(BOOTXX_FFSV2)
	case FS_BSDFFS:
		if (bsdlabel[rootpart].pi_flags & PIF_FFSv2) {
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
#ifdef BOOTXX_LFS
	case FS_BSDLFS:
		bootxxname = BOOTXX_LFS;
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

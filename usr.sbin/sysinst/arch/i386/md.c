/*	$NetBSD: md.c,v 1.34 2022/01/29 16:01:18 martin Exp $ */

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Based on code written by Philip A. Nelson for Piermont Information
 * Systems Inc.
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
 */

/* md.c -- i386 machine specific routines - also used by amd64 */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/exec.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <machine/cpu.h>
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <util.h>
#include <dirent.h>
#include <termios.h>

#include "defs.h"
#include "md.h"
#include "endian.h"
#include "msg_defs.h"
#include "menu_defs.h"

#ifdef NO_LBA_READS		/* for testing */
#undef BIFLAG_EXTINT13
#define BIFLAG_EXTINT13	0
#endif

static struct biosdisk_info *biosdisk = NULL;
static bool uefi_boot;

/* prototypes */

static bool get_bios_info(const char*, struct disk_partitions*, int*, int*, int*);
static int mbr_root_above_chs(daddr_t);
static int md_read_bootcode(const char *, struct mbr_sector *);
static unsigned int get_bootmodel(void);

static int conmib[] = { CTL_MACHDEP, CPU_CONSDEV };

#define	BOOT_PART	(128*(MEG/512))
#define	BOOT_PART_TYPE	PT_EFI_SYSTEM

static const char * uefi_bootloaders[] = {
	"/usr/mdec/bootia32.efi",
	"/usr/mdec/bootx64.efi",
};

void
md_init(void)
{
	char boot_method[100];
	size_t len;

	len = sizeof(boot_method);
	if (sysctlbyname("machdep.bootmethod", boot_method, &len, NULL, 0)
	    != -1) {
		if (strcmp(boot_method, "BIOS") == 0)
			uefi_boot = false;
		else if (strcmp(boot_method, "UEFI") == 0)
			uefi_boot = true;
	}
}

void
md_init_set_status(int flags)
{
	(void)flags;

	/* Default to install same type of kernel as we are running */
	set_kernel_set(get_bootmodel());
}

bool
md_get_info(struct install_partition_desc *install)
{
	int bcyl = 0, bhead = 0, bsec = 0, res;

	if (pm->no_mbr || pm->no_part)
		return true;

again:
	if (pm->parts == NULL) {

		const struct disk_partitioning_scheme *ps =
		    select_part_scheme(pm, NULL, true, NULL);

		if (!ps)
			return false;

		struct disk_partitions *parts =
		   (*ps->create_new_for_disk)(pm->diskdev,
		   0, pm->dlsize, true, NULL);
		if (!parts)
			return false;

		pm->parts = parts;
		if (ps->size_limit > 0 && pm->dlsize > ps->size_limit)
			pm->dlsize = ps->size_limit;
	}

	if (get_bios_info(pm->diskdev, pm->parts, &bcyl, &bhead, &bsec)
	    && pm->parts->pscheme->change_disk_geom != NULL)
		pm->parts->pscheme->change_disk_geom(pm->parts,
		    bcyl, bhead, bsec);
	else
		set_default_sizemult(pm->diskdev, MEG, pm->sectorsize);

	/*
	 * If the selected scheme does not need two-stage partitioning
	 * (like GPT), do not bother to edit the outer partitions.
	 */
	if (pm->parts->pscheme->secondary_partitions == NULL ||
	    pm->parts->pscheme->secondary_scheme == NULL)
		return true;

	if (pm->no_mbr || pm->no_part)
		return true;

	res = edit_outer_parts(pm->parts);
	if (res == 0)
		return false;
	else if (res == 1)
		return true;

	pm->parts->pscheme->destroy_part_scheme(pm->parts);
	pm->parts = NULL;
	goto again;
}

/*
 * md back-end code for menu-driven BSD disklabel editor.
 */
int
md_make_bsd_partitions(struct install_partition_desc *install)
{
	return make_bsd_partitions(install);
}

/*
 * any additional partition validation
 */
bool
md_check_partitions(struct install_partition_desc *install)
{
	int rval;
	char *bootxx;

	/* if booting via UEFI no boot blocks are needed */
	if (uefi_boot)
		return true;

	/* check we have boot code for the root partition type */
	bootxx = bootxx_name(install);
	rval = access(bootxx, R_OK);
	free(bootxx);
	if (rval == 0)
		return true;
	process_menu(MENU_ok, __UNCONST(MSG_No_Bootcode));
	return false;
}

/*
 * hook called before writing new disklabel.
 */
bool
md_pre_disklabel(struct install_partition_desc *install,
    struct disk_partitions *parts)
{

	if (parts->parent == NULL)
		return true;	/* no outer partitions */

	parts = parts->parent;

	msg_display_subst(MSG_dofdisk, 3, parts->disk,
	    msg_string(parts->pscheme->name),
	    msg_string(parts->pscheme->short_name));

	/* write edited "MBR" onto disk. */
	if (!parts->pscheme->write_to_disk(parts)) {
		msg_display(MSG_wmbrfail);
		process_menu(MENU_ok, NULL);
		return false;
	}
	return true;
}

/*
 * hook called after writing disklabel to new target disk.
 */
bool
md_post_disklabel(struct install_partition_desc *install,
    struct disk_partitions *parts)
{
	return true;
}

/*
 * Do all legacy bootblock update/setup here
 */
static int
update_bios_boot(struct install_partition_desc *install, bool use_target_files)
{
	int ret;
	size_t len;
	char boot_options[1024];
	char *bootxx_filename, *p;
	/*
	 * XXX - this code retains a lot of cruft from when we went
	 * to great pains to exclude installboot from the ramdisk
	 * for size reasons and should be rewritten.
	 */
	static const char *consoles[]={
        	"pc", /* CONSDEV_PC */
		"com0", /* CONSDEV_COM0 */
		"com1", /* CONSDEV_COM1 */
		"com2", /* CONSDEV_COM2 */
		"com3", /* CONSDEV_COM3 */
		"com0kbd", /* CONSDEV_COM0KBD */
		"com1kbd", /* CONSDEV_COM1KBD */
		"com2kbd", /* CONSDEV_COM2KBD */
		"com3kbd" /* CONSDEV_COM3KBD */ };
	static struct x86_boot_params boottype =
		{sizeof boottype, 0, 5, 0, 9600, { '\0' }, "", 0};
	struct termios t;
	dev_t condev;

	if (pm == NULL || !pm->no_part) {
		/*
		 * Get console device, should either be ttyE0 or tty0n.
		 * Too hard to double check, so just 'know' the device numbers.
		 */
		len = sizeof condev;
		if (sysctl(conmib, __arraycount(conmib), &condev, &len, NULL,
		    0) != -1 && (condev & ~3) == 0x800) {
			/* Motherboard serial port */
			boottype.bp_consdev = (condev & 3) + 1;
			/* Defaulting the baud rate to that of stdin should
			   suffice */
			if (tcgetattr(0, &t) != -1)
				boottype.bp_conspeed = t.c_ispeed;
		}

		process_menu(MENU_getboottype, &boottype);
		msg_fmt_display(MSG_dobootblks, "%s", pm->diskdev);
		if (boottype.bp_consdev == ~0u)
			/* Use existing bootblocks */
			return 0;
	}

	if (use_target_files)
		ret = cp_within_target("/usr/mdec/boot", "/", 0);
	else
		ret = cp_to_target("/usr/mdec/boot", "/");
	if (ret)
		return ret;
	if (pm && pm->no_part)
		return 0;

	p = bootxx_name(install);
	if (p && use_target_files) {
		bootxx_filename = strdup(target_expand(p));
		free(p);
	} else {
	        bootxx_filename = p;
	}
        if (bootxx_filename != NULL) {
		char rdev[PATH_MAX];

		install->infos[0].parts->pscheme->get_part_device(
		    install->infos[0].parts, install->infos[0].cur_part_id,
		    rdev, sizeof rdev, NULL, raw_dev_name, true, true);

		snprintf(boot_options, sizeof boot_options,
		    "console=%s,speed=%u", consoles[boottype.bp_consdev],
		    boottype.bp_conspeed);
               	ret = run_program(RUN_DISPLAY,
                	    "/usr/sbin/installboot -f -o %s %s %s",
			    boot_options, rdev, bootxx_filename);
                free(bootxx_filename);
        } else {
                ret = -1;
	}

        if (ret != 0)
                process_menu(MENU_ok,
                    __UNCONST("Warning: disk is probably not bootable"));

	return ret;
}

static int
md_post_newfs_bios(struct install_partition_desc *install)
{
	return update_bios_boot(install, false);
}

/*
 * Make sure our bootloader(s) are in the proper directory in the boot
 * boot partition (or update them).
 */
static int
copy_uefi_boot(const struct part_usage_info *boot)
{
	char dev[MAXPATHLEN], path[MAXPATHLEN], src[MAXPATHLEN];
	size_t i;
	int err;

	if (!boot->parts->pscheme->get_part_device(boot->parts,
	    boot->cur_part_id, dev, sizeof(dev), NULL, plain_name, true, true))
		return -1;

	/*
	 * We should have a valid file system on that partition.
	 * Try to mount it and check if there is a /EFI in there.
	 */
	if (boot->mount[0])
		strlcpy(path, boot->mount, sizeof(path));
	else
		strcpy(path, "/mnt");

	if (!(boot->instflags & PUIINST_MOUNT)) {
		make_target_dir(path);
		err = target_mount("", dev, path);
		if (err != 0)
			return err;
	}

	strlcat(path, "/EFI/boot", sizeof(path));
	make_target_dir(path);

	for (i = 0; i < __arraycount(uefi_bootloaders); i++) {
		strcpy(src, target_expand(uefi_bootloaders[i]));
		if (access(src, R_OK) != 0)
			continue;
		err = cp_within_target(uefi_bootloaders[i], path, 0);
		if (err)
			return err;
	}

	return 0;
}

/*
 * Find (U)EFI boot partition and install/update bootloaders
 */
static int
update_uefi_boot_code(struct install_partition_desc *install)
{
	size_t i, boot_part;

	boot_part = ~0U;
	for (i = 0; i < install->num; i++) {
		if (!(install->infos[i].instflags & PUIINST_BOOT))
			continue;
		boot_part = i;
		break;
	}
	if (boot_part == ~0U) {
		/*
		 * Didn't find an explicitly marked boot partition,
		 * check if we have any EFI System Partitions and
		 * use the first.
		 */
		for (i = 0; i < install->num; i++) {
			if (install->infos[i].type != PT_EFI_SYSTEM)
				continue;
			boot_part = i;
			break;
		}
	}

	if (boot_part < install->num)
		return copy_uefi_boot(&install->infos[boot_part]);

	return -1;	/* no EFI boot partition found */
}
 
/*
 * Find bootloader options and update bootloader
 */
static int
update_bios_boot_code(struct install_partition_desc *install)
{
	return update_bios_boot(install, true);
}

static int
update_boot_code(struct install_partition_desc *install)
{
	return uefi_boot ?
	    update_uefi_boot_code(install)
	    : update_bios_boot_code(install);
}

static int
md_post_newfs_uefi(struct install_partition_desc *install)
{
	return update_uefi_boot_code(install);
}

/*
 * hook called after upgrade() or install() has finished setting
 * up the target disk but immediately before the user is given the
 * ``disks are now set up'' message.
 */
int
md_post_newfs(struct install_partition_desc *install)
{

	return uefi_boot ? md_post_newfs_uefi(install)
	    : md_post_newfs_bios(install);
}

int
md_post_extract(struct install_partition_desc *install, bool upgrade)
{
	if (upgrade)
		update_boot_code(install);

#if defined(__amd64__)
	if (get_kernel_set() == SET_KERNEL_2) {
		int ret;

		ret = cp_within_target("/usr/mdec/prekern", "/prekern", 0);
		if (ret)
			return ret;
	}
#endif

	return 0;
}

void
md_cleanup_install(struct install_partition_desc *install)
{
	size_t len;
	dev_t condev;

#ifndef DEBUG
	enable_rc_conf();
	add_rc_conf("wscons=YES\n");

# if defined(__i386__) && defined(SET_KERNEL_TINY)
	/*
	 * For GENERIC_TINY, do not enable any extra screens or wsmux.
	 * Otherwise, run getty on 4 VTs.
	 */
	if (get_kernel_set() == SET_KERNEL_TINY)
		run_program(RUN_CHROOT,
                            "sed -an -e '/^screen/s/^/#/;/^mux/s/^/#/;"
			    "H;$!d;g;w /etc/wscons.conf' /etc/wscons.conf");
	else
# endif
		run_program(RUN_CHROOT,
			    "sed -an -e '/^ttyE[1-9]/s/off/on/;"
			    "H;$!d;g;w /etc/ttys' /etc/ttys");

#endif

	/*
	 * Get console device, should either be ttyE0 or tty0n.
	 * Too hard to double check, so just 'know' the device numbers.
	 */
	len = sizeof condev;
	if (sysctl(conmib, __arraycount(conmib), &condev, &len, NULL, 0) != -1
	    && (condev & ~3) != 0x800) {

		/*
		 * Current console is not com*, assume ttyE*.
		 * Modify /etc/ttys to use wsvt25 for all ports.
		 */

		run_program(RUN_CHROOT,
			    "sed -an -e 's/vt100/wsvt25/g;"
			    "H;$!d;g;w  /etc/ttys' /etc/ttys");
	}
}

int
md_pre_update(struct install_partition_desc *install)
{
	return 1;
}

/* Upgrade support */
int
md_update(struct install_partition_desc *install)
{
	return 1;
}

int
md_check_mbr(struct disk_partitions *parts, mbr_info_t *mbri, bool quiet)
{
	mbr_info_t *ext;
	struct mbr_partition *p;
	const char *bootcode;
	daddr_t inst_start, inst_size;
	int i, names, fl, ofl;
#define	ACTIVE_FOUND	0x0100
#define	NETBSD_ACTIVE	0x0200
#define	NETBSD_NAMED	0x0400
#define	ACTIVE_NAMED	0x0800

	root_limit = 0;
	if (parts->pscheme->guess_install_target == NULL ||
	    !parts->pscheme->guess_install_target(parts, &inst_start,
	    &inst_size)) {
		inst_start = parts->disk_start;
		inst_size = parts->disk_size;
	}

	if (biosdisk != NULL && (biosdisk->bi_flags & BIFLAG_EXTINT13) == 0) {
		if (mbr_root_above_chs(inst_start)) {
			if (quiet)
				return 0;
			msg_display(MSG_partabovechs);
			if (!ask_noyes(NULL))
				return 1;
			/* The user is shooting themselves in the foot here...*/
		} else {
			if (parts->pscheme->size_limit)
				root_limit = min(parts->pscheme->size_limit,
				    parts->disk_size);
			else
				root_limit = parts->disk_size;
		}
	}

	/*
	 * Ensure the install partition (at sector inst_start) and the active
	 * partition are bootable.
	 * Determine whether the bootselect code is needed.
	 * Note that MBR_BS_NEWMBR is always set, so we ignore it!
	 */
	fl = 0;
	names = 0;
	for (ext = mbri; ext != NULL; ext = ext->extended) {
		p = ext->mbr.mbr_parts;
		for (i = 0; i < MBR_PART_COUNT; p++, i++) {
			if (p->mbrp_flag == MBR_PFLAG_ACTIVE) {
				fl |= ACTIVE_FOUND;
			    if (ext->sector + p->mbrp_start == inst_start)
				fl |= NETBSD_ACTIVE;
			}
			if (ext->mbrb.mbrbs_nametab[i][0] == 0) {
				/* No bootmenu label... */
				if (ext->sector == 0)
					continue;
				if (ext->sector + p->mbrp_start == inst_start)
					/*
					 * Have installed into an extended ptn
					 * force name & bootsel...
					 */
					names++;
				continue;
			}
			/* Partition has a bootmenu label... */
			if (ext->sector != 0)
				fl |= MBR_BS_EXTLBA;
			if (ext->sector + p->mbrp_start == inst_start)
				fl |= NETBSD_NAMED;
			else if (p->mbrp_flag == MBR_PFLAG_ACTIVE)
				fl |= ACTIVE_NAMED;
			else
				names++;
		}
	}
	if (!(fl & ACTIVE_FOUND))
		fl |= NETBSD_ACTIVE;
	if (fl & NETBSD_NAMED && fl & NETBSD_ACTIVE)
		fl |= ACTIVE_NAMED;

	if ((names > 0 || !(fl & NETBSD_ACTIVE)) &&
	    (!(fl & NETBSD_NAMED) || !(fl & ACTIVE_NAMED))) {
		/*
		 * There appear to be multiple bootable partitions, but they
		 * don't all have bootmenu texts.
		 */
		if (quiet)
			return 0;

		msg_display(MSG_missing_bootmenu_text);
		if (ask_yesno(NULL))
			return 1;
	}

	if ((fl & MBR_BS_EXTLBA) &&
	    (biosdisk == NULL || !(biosdisk->bi_flags & BIFLAG_EXTINT13))) {
		/* Need unsupported LBA reads to read boot sectors */
		if (quiet)
			return 0;

		msg_display(MSG_no_extended_bootmenu);
		if (!ask_noyes(NULL))
			return 1;
	}

	/* Sort out the name of the mbr code we need */
	if (names > 1 ||
	    (parts->num_part > 1 && (fl & (NETBSD_NAMED | ACTIVE_NAMED)))) {
		/* Need bootselect code */
		fl |= MBR_BS_ACTIVE;
		bootcode = fl & MBR_BS_EXTLBA ? _PATH_BOOTEXT : _PATH_BOOTSEL;
	} else {
		bootcode = _PATH_MBR;
	}

	fl &=  MBR_BS_ACTIVE | MBR_BS_EXTLBA;

	/* Look at what is installed */
	ofl = mbri->mbrb.mbrbs_flags;
	if (ofl == 0) {
		/* Check there is some bootcode at all... */
		if (mbri->mbr.mbr_magic != htole16(MBR_MAGIC) ||
		    mbri->mbr.mbr_jmpboot[0] == 0 ||
		    mbr_root_above_chs(inst_start))
			/* Existing won't do, force update */
			fl |= MBR_BS_NEWMBR;
	}
	ofl = mbri->oflags & (MBR_BS_ACTIVE | MBR_BS_EXTLBA);

	if (!quiet) {
		if (fl & ~ofl || (fl == 0 && ofl & MBR_BS_ACTIVE)) {
			/* Existing boot code isn't the right one... */
			if (fl & MBR_BS_ACTIVE)
				msg_display(MSG_installbootsel);
			else
				msg_display(MSG_installmbr);
		} else
			/* Existing code would (probably) be ok */
			msg_display(MSG_updatembr);

		if (!ask_yesno(NULL))
			/* User doesn't want to update mbr code */
			return 2;
	}

	if (md_read_bootcode(bootcode, &mbri->mbr) == 0)
		/* update suceeded - to memory copy */
		return 2;

	/* This shouldn't happen since the files are in the floppy fs... */
	msg_fmt_display("Can't find %s", "%s", bootcode);
	return ask_reedit(parts);
}

bool
md_parts_use_wholedisk(struct disk_partitions *parts)
{
	struct disk_part_info boot_part = {
		.size = BOOT_PART,
		.fs_type = FS_MSDOS, .fs_sub_type = MBR_PTYPE_FAT32L,
	};

	if (!uefi_boot)
		return parts_use_wholedisk(parts, 0, NULL);

	boot_part.nat_type = parts->pscheme->get_generic_part_type(
	    PT_EFI_SYSTEM);

	return parts_use_wholedisk(parts, 1, &boot_part);
}

static bool
get_bios_info(const char *dev, struct disk_partitions *parts, int *bcyl,
    int *bhead, int *bsec)
{
	static struct disklist *disklist = NULL;
	static int mib[2] = {CTL_MACHDEP, CPU_DISKINFO};
	int i;
	size_t len;
	struct biosdisk_info *bip;
	struct nativedisk_info *nip = NULL, *nat;
	int cyl, head, sec;

	if (disklist == NULL) {
		if (sysctl(mib, 2, NULL, &len, NULL, 0) < 0)
			goto nogeom;
		disklist = malloc(len);
		if (disklist == NULL) {
			fprintf(stderr, "Out of memory\n");
			return false;
		}
		sysctl(mib, 2, disklist, &len, NULL, 0);
	}

	for (i = 0; i < disklist->dl_nnativedisks; i++) {
		nat = &disklist->dl_nativedisks[i];
		if (!strcmp(dev, nat->ni_devname)) {
			nip = nat;
			break;
		}
	}
	if (nip == NULL || nip->ni_nmatches == 0) {
nogeom:
		if (nip != NULL)
			msg_fmt_display(MSG_nobiosgeom, "%d%d%d",
			    pm->dlcyl, pm->dlhead, pm->dlsec);
		if (guess_biosgeom_from_parts(parts, &cyl, &head, &sec) >= 0
		    && nip != NULL)
		{
			msg_fmt_display_add(MSG_biosguess, "%d%d%d",
			    cyl, head, sec);
		}
		biosdisk = NULL;
	} else {
		guess_biosgeom_from_parts(parts, &cyl, &head, &sec);
		if (nip->ni_nmatches == 1) {
			bip = &disklist->dl_biosdisks[nip->ni_biosmatches[0]];
			msg_display(MSG_onebiosmatch);
			msg_table_add(MSG_onebiosmatch_header);
			msg_fmt_table_add(MSG_onebiosmatch_row, "%d%d%d%d%u%u",
			    bip->bi_dev, bip->bi_cyl, bip->bi_head, bip->bi_sec,
			    (unsigned)bip->bi_lbasecs,
			    (unsigned)(bip->bi_lbasecs / (1000000000 / 512)));
			msg_display_add(MSG_biosgeom_advise);
			biosdisk = bip;
			process_menu(MENU_biosonematch, &biosdisk);
		} else {
			msg_display(MSG_biosmultmatch);
			msg_table_add(MSG_biosmultmatch_header);
			for (i = 0; i < nip->ni_nmatches; i++) {
				bip = &disklist->dl_biosdisks[
							nip->ni_biosmatches[i]];
				msg_fmt_table_add(MSG_biosmultmatch_row, 
				    "%d%d%d%d%d%u%u", i,
				    bip->bi_dev, bip->bi_cyl, bip->bi_head,
				    bip->bi_sec, (unsigned)bip->bi_lbasecs,
				    (unsigned)bip->bi_lbasecs/(1000000000/512));
			}
			process_menu(MENU_biosmultmatch, &i);
			if (i == -1)
				biosdisk = NULL;
			else
				biosdisk = &disklist->dl_biosdisks[
							nip->ni_biosmatches[i]];
		}
	}
	if (biosdisk == NULL) {
		*bcyl = cyl;
		*bhead = head;
		*bsec = sec;
		if (nip != NULL)
			set_bios_geom(parts, bcyl, bhead, bsec);
	} else {
		*bcyl = biosdisk->bi_cyl;
		*bhead = biosdisk->bi_head;
		*bsec = biosdisk->bi_sec;
	}
	return true;
}

static int
mbr_root_above_chs(daddr_t ptstart)
{
	return ptstart + (daddr_t)DEFROOTSIZE * (daddr_t)(MEG / 512)
	    >= pm->max_chs;
}

/* returns false if no write-back of parts is required */
bool
md_mbr_update_check(struct disk_partitions *parts, mbr_info_t *mbri)
{
	struct mbr_partition *mbrp;
	int i, netbsdpart = -1, oldbsdpart = -1, oldbsdcount = 0;

	if (pm->no_mbr || pm->no_part)
		return false;

	mbrp = &mbri->mbr.mbr_parts[0];

	for (i = 0; i < MBR_PART_COUNT; i++) {
		if (mbrp[i].mbrp_type == MBR_PTYPE_386BSD) {
			oldbsdpart = i;
			oldbsdcount++;
		} else if (mbrp[i].mbrp_type == MBR_PTYPE_NETBSD)
			netbsdpart = i;
	}

	if (netbsdpart == -1 && oldbsdcount == 1) {
		mbrp[oldbsdpart].mbrp_type = MBR_PTYPE_NETBSD;
		return true;
	}

	return false;
}

/*
 * Read MBR code from a file.
 * The existing partition table and bootselect configuration is kept.
 */
static int
md_read_bootcode(const char *path, struct mbr_sector *mbrs)
{
	int fd;
	struct stat st;
	size_t len;
	struct mbr_sector new_mbr;
	uint32_t dsn;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;

	if (fstat(fd, &st) < 0 || st.st_size != sizeof *mbrs) {
		close(fd);
		return -1;
	}

	if (read(fd, &new_mbr, sizeof new_mbr) != sizeof new_mbr) {
		close(fd);
		return -1;
	}
	close(fd);

	if (new_mbr.mbr_bootsel_magic != htole16(MBR_BS_MAGIC))
		return -1;

	if (mbrs->mbr_bootsel_magic == htole16(MBR_BS_MAGIC)) {
		len = offsetof(struct mbr_sector, mbr_bootsel);
	} else
		len = offsetof(struct mbr_sector, mbr_parts);

	/* Preserve the 'drive serial number' - especially for Vista */
	dsn = mbrs->mbr_dsn;
	memcpy(mbrs, &new_mbr, len);
	mbrs->mbr_dsn = dsn;

	/* Keep flags from object file - indicate the properties */
	mbrs->mbr_bootsel.mbrbs_flags = new_mbr.mbr_bootsel.mbrbs_flags;
	mbrs->mbr_magic = htole16(MBR_MAGIC);

	return 0;
}

static unsigned int
get_bootmodel(void)
{
#if defined(__i386__)
	struct utsname ut;
#ifdef DEBUG
	char *envstr;

	envstr = getenv("BOOTMODEL");
	if (envstr != NULL)
		return atoi(envstr);
#endif

	if (uname(&ut) < 0)
		ut.version[0] = 0;

#if defined(SET_KERNEL_TINY)
	if (strstr(ut.version, "TINY") != NULL)
		return SET_KERNEL_TINY;
#endif
#if defined(SET_KERNEL_PS2)
	if (strstr(ut.version, "PS2") != NULL)
		return SET_KERNEL_PS2;
#endif
#endif
	return SET_KERNEL_GENERIC;
}


int
md_pre_mount(struct install_partition_desc *install, size_t ndx)
{
	return 0;
}

#ifdef HAVE_GPT
/*
 * New GPT partitions have been written, update bootloader or remember
 * data untill needed in md_post_newfs
 */
bool
md_gpt_post_write(struct disk_partitions *parts, part_id root_id,
    bool root_is_new, part_id efi_id, bool efi_is_new)
{
	struct disk_part_info info;

	if (root_id != NO_PART) {
		/* we always update the gpt boot record for now */
		if (!parts->pscheme->get_part_info(parts, root_id, &info))
			return false;
		if (run_program(RUN_SILENT, "gpt biosboot -b %" PRIu64 " %s",
		    info.start, parts->disk) != 0)
			return false;
	}

	return true;
}
#endif

/*
 * When we do an UEFI install, we have completely different default
 * partitions and need to adjust the description at runtime.
 */
void
x86_md_part_defaults(struct pm_devs *cur_pm, struct part_usage_info **partsp,
    size_t *num_usage_infos)
{
	static const struct part_usage_info uefi_boot_part =
	{
		.size = BOOT_PART,
		.type = BOOT_PART_TYPE,
		.instflags = PUIINST_NEWFS|PUIINST_BOOT,
		.fs_type = FS_MSDOS, .fs_version = MBR_PTYPE_FAT32L,
		.flags = PUIFLAG_ADD_OUTER,
	};

	struct disk_partitions *parts;
	struct part_usage_info *new_usage, *boot;
	struct disk_part_info info;
	size_t num;
	part_id pno;

	if (!uefi_boot)
		return;		/* legacy defaults apply */

	/*
	 * Insert a UEFI boot partition at the beginning of the array
	 */

	/* create space for new description */
	num = *num_usage_infos + 1;
	new_usage = realloc(*partsp, sizeof(*new_usage)*num);
	if (new_usage == NULL)
		return;
	*partsp = new_usage;
	*num_usage_infos = num;
	boot = new_usage;
	memmove(boot+1, boot, sizeof(*boot)*(num-1));
	*boot = uefi_boot_part;

	/*
	 * Check if the UEFI partition already exists
	 */
	parts = pm->parts;
	if (parts->parent != NULL)
		parts = parts->parent;
	for (pno = 0; pno < parts->num_part; pno++) {
		if (!parts->pscheme->get_part_info(parts, pno, &info))
			continue;
		if (info.nat_type->generic_ptype != boot->type)
			continue;
		boot->flags &= ~PUIFLAG_ADD_OUTER;
		boot->flags |= PUIFLG_IS_OUTER|PUIFLG_ADD_INNER;
		boot->size = info.size;
		boot->cur_start = info.start;
		boot->cur_flags = info.flags;
		break;
	}
}

/* no need to install bootblock if installing for UEFI */
bool
x86_md_need_bootblock(struct install_partition_desc *install)
{

	return !uefi_boot;
}

/*	$NetBSD: md.c,v 1.118 2008/01/28 02:47:14 rumble Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Piermont Information Systems Inc.
 * 4. The name of Piermont Information Systems Inc. may not be used to endorse
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

/* md.c -- Machine specific code for i386 */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/exec.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <machine/cpu.h>
#include <stdio.h>
#include <stddef.h>
#include <util.h>
#include <dirent.h>
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

/* prototypes */

static int mbr_root_above_chs(void);
static void md_upgrade_mbrtype(void);
static int md_read_bootcode(const char *, struct mbr_sector *);
static unsigned int get_bootmodel(void);
static char *md_bootxx_name(void);


int
md_get_info(void)
{
	mbr_info_t *ext;
	struct mbr_partition *p;
	const char *bootcode;
	int i;
	int names, fl, ofl;
#define	ACTIVE_FOUND	0x0100
#define	NETBSD_ACTIVE	0x0200
#define	NETBSD_NAMED	0x0400
#define	ACTIVE_NAMED	0x0800

	if (no_mbr)
		return 1;

	if (read_mbr(diskdev, &mbr) < 0)
		memset(&mbr.mbr, 0, sizeof mbr.mbr - 2);
	md_bios_info(diskdev);

edit:
	if (edit_mbr(&mbr) == 0)
		return 0;

	root_limit = 0;
	if (biosdisk == NULL || !(biosdisk->bi_flags & BIFLAG_EXTINT13)) {
		if (mbr_root_above_chs()) {
			msg_display(MSG_partabovechs);
			process_menu(MENU_noyes, NULL);
			if (!yesno)
				goto edit;
			/* The user is shooting themselves in the foot here...*/
		} else
			root_limit = bcyl * bhead * bsec;
	}

	/*
	 * Ensure the install partition (at sector ptstart) and the active
	 * partition are bootable.
	 * Determine whether the bootselect code is needed.
	 * Note that MBR_BS_NEWMBR is always set, so we ignore it!
	 */
	fl = 0;
	names = 0;
	for (ext = &mbr; ext != NULL; ext = ext->extended) {
		p = ext->mbr.mbr_parts;
		for (i = 0; i < MBR_PART_COUNT; p++, i++) {
			if (p->mbrp_flag == MBR_PFLAG_ACTIVE) {
				fl |= ACTIVE_FOUND;
			    if (ext->sector + p->mbrp_start == ptstart)
				fl |= NETBSD_ACTIVE;
			}
			if (ext->mbrb.mbrbs_nametab[i][0] == 0) {
				/* No bootmenu label... */
				if (ext->sector == 0)
					continue;
				if (ext->sector + p->mbrp_start == ptstart)
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
			if (ext->sector + p->mbrp_start == ptstart)
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
		msg_display(MSG_missing_bootmenu_text);
		process_menu(MENU_yesno, NULL);
		if (yesno)
			goto edit;
	}

	if ((fl & MBR_BS_EXTLBA) &&
	    (biosdisk == NULL || !(biosdisk->bi_flags & BIFLAG_EXTINT13))) {
		/* Need unsupported LBA reads to read boot sectors */
		msg_display(MSG_no_extended_bootmenu);
		process_menu(MENU_noyes, NULL);
		if (!yesno)
			goto edit;
	}

	/* Sort out the name of the mbr code we need */
	if (names > 0 || fl & (NETBSD_NAMED | ACTIVE_NAMED)) {
		/* Need bootselect code */
		fl |= MBR_BS_ACTIVE;
		bootcode = fl & MBR_BS_EXTLBA ? _PATH_BOOTEXT : _PATH_BOOTSEL;
	} else
		bootcode = _PATH_MBR;

	fl &=  MBR_BS_ACTIVE | MBR_BS_EXTLBA;

	/* Look at what is installed */
	ofl = mbr.mbrb.mbrbs_flags;
	if (ofl == 0) {
		/* Check there is some bootcode at all... */
		if (mbr.mbr.mbr_magic != htole16(MBR_MAGIC) ||
		    mbr.mbr.mbr_jmpboot[0] == 0 ||
		    mbr_root_above_chs())
			/* Existing won't do, force update */
			fl |= MBR_BS_NEWMBR;
	}
	ofl = mbr.oflags & (MBR_BS_ACTIVE | MBR_BS_EXTLBA);

	if (fl & ~ofl || (fl == 0 && ofl & MBR_BS_ACTIVE)) {
		/* Existing boot code isn't the right one... */
		if (fl & MBR_BS_ACTIVE)
			msg_display(MSG_installbootsel);
		else
			msg_display(MSG_installmbr);
	} else
		/* Existing code would (probably) be ok */
		msg_display(MSG_updatembr);

	process_menu(MENU_yesno, NULL);
	if (!yesno)
		/* User doesn't want to update mbr code */
		return 1;

	if (md_read_bootcode(bootcode, &mbr.mbr) == 0)
		/* update suceeded - to memory copy */
		return 1;

	/* This shouldn't happen since the files are in the floppy fs... */
	msg_display("Can't find %s", bootcode);
	process_menu(MENU_yesno, NULL);

	return 1;
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

int
md_pre_disklabel(void)
{
	if (no_mbr)
		return 0;

	msg_display(MSG_dofdisk);

	/* write edited MBR onto disk. */
	if (write_mbr(diskdev, &mbr, 1) != 0) {
		msg_display(MSG_wmbrfail);
		process_menu(MENU_ok, NULL);
		return 1;
	}
	return 0;
}

int
md_post_disklabel(void)
{
	if (get_ramsize() <= 32)
		set_swap(diskdev, bsdlabel);

	return 0;
}

int
md_post_newfs(void)
{
	int ret;
	size_t len;
	int td, sd;
	char bootxx[8192 + 4];
	char *bootxx_filename;
	/*
	 * XXX - should either find some way to pull this automatically
	 * from sys/arch/i386/stand/lib/boot_params.S, or just bite the
	 * bullet and include /sbin/installboot on the ramdisk
	 */
	static struct x86_boot_params boottype =
		{sizeof boottype, 0, 5, 0, 9600, { '\0' }, "", 0};
	static int conmib[] = {CTL_MACHDEP, CPU_CONSDEV};
	struct termios t;
	dev_t condev;
#define bp (*(struct x86_boot_params *)(bootxx + 512 * 2 + 8))

	/*
	 * Get console device, should either be ttyE0 or tty0n.
	 * Too hard to double check, so just 'know' the device numbers.
	 */
	len = sizeof condev;
	if (sysctl(conmib, nelem(conmib), &condev, &len, NULL, 0) != -1
	    && (condev & ~3) == 0x800) {
		/* Motherboard serial port */
		boottype.bp_consdev = (condev & 3) + 1;
		/* Defaulting the baud rate to that of stdin should suffice */
		if (tcgetattr(0, &t) != -1)
			boottype.bp_conspeed = t.c_ispeed;
	}

	process_menu(MENU_getboottype, &boottype);
	msg_display(MSG_dobootblks, diskdev);
	if (bp.bp_consdev == ~0)
		return 0;

	ret = cp_to_target("/usr/mdec/boot", "/boot");
	if (ret)
		return ret;

	/* Copy bootstrap in by hand - /sbin/installboot explodes ramdisks */
	ret = 1;

	snprintf(bootxx, sizeof bootxx, "/dev/r%s%c", diskdev, 'a' + rootpart);
	td = open(bootxx, O_RDWR, 0);
	bootxx_filename = md_bootxx_name();
	if (bootxx_filename != NULL) {
		sd = open(bootxx_filename, O_RDONLY);
		free(bootxx_filename);
	} else
		sd = -1;
	if (td == -1 || sd == -1)
		goto bad_bootxx;
	len = read(sd, bootxx, sizeof bootxx);
	if (len < 2048 || len > 8192)
		goto bad_bootxx;

	if (*(uint32_t *)(bootxx + 512 * 2 + 4) != X86_BOOT_MAGIC_1)
		goto bad_bootxx;

	boottype.bp_length = bp.bp_length;
	memcpy(&bp, &boottype, min(boottype.bp_length, sizeof boottype));

	if (pwrite(td, bootxx, 512, 0) != 512)
		goto bad_bootxx;
	len -= 512 * 2;
	if (pwrite(td, bootxx + 512 * 2, len, 2 * (off_t)512) != len)
		goto bad_bootxx;
	ret = 0;

    bad_bootxx:
	close(td);
	close(sd);

	return ret;
}

int
md_copy_filesystem(void)
{
	return 0;
}


int
md_make_bsd_partitions(void)
{

	return make_bsd_partitions();
}

int
md_pre_update(void)
{
	if (get_ramsize() <= 8)
		set_swap(diskdev, NULL);
	return 1;
}

/*
 * any additional partition validation
 */
int
md_check_partitions(void)
{
	int rval;
	char *bootxx;

	/* check we have boot code for the root partition type */
	bootxx = md_bootxx_name();
	rval = access(bootxx, R_OK);
	free(bootxx);
	if (rval == 0)
		return 1;
	process_menu(MENU_ok, deconst(MSG_No_Bootcode));
	return 0;
}


/* Upgrade support */
int
md_update(void)
{
	move_aout_libs();
	/* endwin(); */
	md_copy_filesystem();
	md_post_newfs();
	md_upgrade_mbrtype();
	wrefresh(curscr);
	wmove(stdscr, 0, 0);
	wclear(stdscr);
	wrefresh(stdscr);
	return 1;
}

void
md_upgrade_mbrtype(void)
{
	struct mbr_partition *mbrp;
	int i, netbsdpart = -1, oldbsdpart = -1, oldbsdcount = 0;

	if (no_mbr)
		return;

	if (read_mbr(diskdev, &mbr) < 0)
		return;

	mbrp = &mbr.mbr.mbr_parts[0];

	for (i = 0; i < MBR_PART_COUNT; i++) {
		if (mbrp[i].mbrp_type == MBR_PTYPE_386BSD) {
			oldbsdpart = i;
			oldbsdcount++;
		} else if (mbrp[i].mbrp_type == MBR_PTYPE_NETBSD)
			netbsdpart = i;
	}

	if (netbsdpart == -1 && oldbsdcount == 1) {
		mbrp[oldbsdpart].mbrp_type = MBR_PTYPE_NETBSD;
		write_mbr(diskdev, &mbr, 0);
	}
}



void
md_cleanup_install(void)
{

	enable_rc_conf();
	
	add_rc_conf("wscons=YES\n");

#if defined(__i386__) && defined(SET_KERNEL_TINY)
	/*
	 * For GENERIC_TINY, do not enable any extra screens or wsmux.
	 * Otherwise, run getty on 4 VTs.
	 */
	if (get_kernel_set() == SET_KERNEL_TINY)
		run_program(RUN_CHROOT,
                            "sed -an -e '/^screen/s/^/#/;/^mux/s/^/#/;"
			    "H;$!d;g;w /etc/wscons.conf' /etc/wscons.conf");
	else
#endif
		run_program(RUN_CHROOT,
			    "sed -an -e '/^ttyE[1-9]/s/off/on/;"
			    "H;$!d;g;w /etc/ttys' /etc/ttys");

	run_program(0, "rm -f %s", target_expand("/sysinst"));
	run_program(0, "rm -f %s", target_expand("/.termcap"));
	run_program(0, "rm -f %s", target_expand("/.profile"));
}

int
md_bios_info(dev)
	char *dev;
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
			return -1;
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
		msg_display(MSG_nobiosgeom, dlcyl, dlhead, dlsec);
		if (guess_biosgeom_from_mbr(&mbr, &cyl, &head, &sec) >= 0)
			msg_display_add(MSG_biosguess, cyl, head, sec);
		biosdisk = NULL;
	} else {
		guess_biosgeom_from_mbr(&mbr, &cyl, &head, &sec);
		if (nip->ni_nmatches == 1) {
			bip = &disklist->dl_biosdisks[nip->ni_biosmatches[0]];
			msg_display(MSG_onebiosmatch);
			msg_table_add(MSG_onebiosmatch_header);
			msg_table_add(MSG_onebiosmatch_row, bip->bi_dev,
			    bip->bi_cyl, bip->bi_head, bip->bi_sec,
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
				msg_table_add(MSG_biosmultmatch_row, i,
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
	if (biosdisk == NULL)
		set_bios_geom(cyl, head, sec);
	else {
		bcyl = biosdisk->bi_cyl;
		bhead = biosdisk->bi_head;
		bsec = biosdisk->bi_sec;
	}
	return 0;
}

static int
mbr_root_above_chs(void)
{

	return ptstart + DEFROOTSIZE * (MEG / 512) >= bcyl * bhead * bsec;
}

unsigned int
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

void
md_init(void)
{
}

void
md_init_set_status(int minimal)
{
	(void)minimal;

	/* Default to install same type of kernel as we are running */
	set_kernel_set(get_bootmodel());
}

static char *
md_bootxx_name(void)
{
	int fstype;
	const char *bootfs = 0;
	char *bootxx;

	/* check we have boot code for the root partition type */
	fstype = bsdlabel[rootpart].pi_fstype;
	if (fstype == FS_BSDFFS)
		if (bsdlabel[rootpart].pi_flags & PIF_FFSv2)
			bootfs = "ffsv2";
		else
			bootfs = "ffsv1";
	else if (fstype == FS_BSDLFS)
			bootfs = "lfsv2";
	else
		bootfs = mountnames[fstype];

	if (bootfs == NULL)
		return NULL;

	asprintf(&bootxx, "/usr/mdec/bootxx_%s", bootfs);
	return bootxx;
}

int
md_post_extract(void)
{
	return 0;
}

int
md_check_mbr(mbr_info_t *mbri)
{
	return 2;
}

int
md_mbr_use_wholedisk(mbr_info_t *mbri)
{
	return mbr_use_wholedisk(mbri);
}

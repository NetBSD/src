/*	$NetBSD: md.c,v 1.81 2003/06/03 11:54:52 dsl Exp $ */

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

mbr_sector_t mbr;
int mbr_len;
int c1024_resp;
struct disklist *disklist = NULL;
struct nativedisk_info *nativedisk;
struct biosdisk_info *biosdisk = NULL;
int netbsd_mbr_installed = 0;

/* prototypes */

static int md_read_bootcode(const char *, mbr_sector_t *);
static int count_mbr_parts(struct mbr_partition *);
static int mbr_part_above_chs(struct mbr_partition *);
static int mbr_partstart_above_chs(struct mbr_partition *);
static void md_upgrade_mbrtype(void);
#if defined(__i386__)
static char *get_bootmodel(void);
#endif


int
md_get_info(void)
{
	read_mbr(diskdev, &mbr, sizeof mbr);
	if (!valid_mbr(&mbr)) {
		memset(&mbr, 0, sizeof mbr);
		/* XXX check result and give up if < 0 */
		mbr_len = md_read_bootcode(_PATH_MBR, &mbr);
		netbsd_mbr_installed = 1;
	} else
		mbr_len = MBR_SECSIZE;
	md_bios_info(diskdev);

edit:
	edit_mbr(&mbr);

	if (mbr_part_above_chs(part) &&
	    (biosdisk == NULL || !(biosdisk->bi_flags & BIFLAG_EXTINT13))) {
		msg_display(MSG_partabovechs);
		process_menu(MENU_noyes, NULL);
		if (!yesno)
			goto edit;
	}

	if (count_mbr_parts(part) > 1) {
		msg_display(MSG_installbootsel);
		process_menu(MENU_yesno, NULL);
		if (yesno) {
			mbr_len = md_read_bootcode(_PATH_BOOTSEL, &mbr);
			configure_bootsel();
			netbsd_mbr_installed = 2;
		} else {
			msg_display(MSG_installnormalmbr);
			process_menu(MENU_yesno, NULL);
			if (yesno) {
				mbr_len = md_read_bootcode(_PATH_MBR, &mbr);
				netbsd_mbr_installed = 1;
			}
		}
	} else {
		mbr_len = md_read_bootcode(_PATH_MBR, &mbr);
		netbsd_mbr_installed = 1;
	}

	if (mbr_partstart_above_chs(part) && netbsd_mbr_installed == 0) {
		msg_display(MSG_installmbr);
		process_menu(MENU_yesno, NULL);
		if (yesno) {
			mbr_len = md_read_bootcode(_PATH_MBR, &mbr);
			netbsd_mbr_installed = 1;
		}
	}

	return 1;
}

/*
 * Read MBR code from a file.
 * The existing partition table and bootselect configuration is kept.
 */
static int
md_read_bootcode(const char *path, mbr_sector_t *mbr)
{
	int fd, cc;
	struct stat st;
	size_t len;
	mbr_sector_t new_mbr;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;

	if (fstat(fd, &st) < 0 || st.st_size != sizeof *mbr) {
		close(fd);
		return -1;
	}

	if (read(fd, &new_mbr, sizeof new_mbr) != sizeof new_mbr) {
		close(fd);
		return -1;
	}

	if (mbr->mbr_bootsel.mbrb_magic == native_to_le16(MBR_MAGIC)) {
		len = offsetof(mbr_sector_t, mbr_bootsel);
		if (!(mbr->mbr_bootsel.mbrb_flags & BFL_NEWMBR))
			/*
			 * Meaning of keys has changed, force a sensible
			 * default (old code didn't preseve the answer).
			 */
			mbr->mbr_bootsel.mbrb_defkey = SCAN_ENTER;
	} else
		len = offsetof(mbr_sector_t, mbr_parts);

	memcpy(mbr, &new_mbr, len);
	/* Keep flags from object file - indicate the properties */
	mbr->mbr_bootsel.mbrb_flags = new_mbr.mbr_bootsel.mbrb_flags;
	mbr->mbr_signature = native_to_le16(MBR_MAGIC);

	close(fd);

	return (cc + MBR_MAGICOFF);
}

int
md_pre_disklabel(void)
{
	msg_display(MSG_dofdisk);

	/* write edited MBR onto disk. */
	if (write_mbr(diskdev, &mbr, sizeof mbr, 1) != 0) {
		msg_display(MSG_wmbrfail);
		process_menu(MENU_ok, NULL);
		return 1;
	}
	md_upgrade_mbrtype();
	return 0;
}

int
md_post_disklabel(void)
{
	if (rammb <= 32)
		set_swap(diskdev, bsdlabel, 1);

	return 0;
}

int
md_post_newfs(void)
{
	struct stat sb;
	int ret;

	/* boot blocks ... */
	ret = stat("/usr/mdec/biosboot_com0_9600.sym", &sb);
	if ((ret != -1) && (sb.st_mode & S_IFREG)) {
		msg_display(MSG_getboottype);
		process_menu(MENU_getboottype, NULL);
	}
	msg_display(MSG_dobootblks, diskdev);
	if (!strncmp(boottype, "serial", 6))
	        return run_prog(RUN_DISPLAY, NULL,
	            "/usr/mdec/installboot -v /usr/mdec/biosboot_com0_%s.sym /dev/r%sa",
	            boottype + 6, diskdev);
	else
	        return run_prog(RUN_DISPLAY, NULL,
	            "/usr/mdec/installboot -v /usr/mdec/biosboot.sym /dev/r%sa",
	            diskdev);
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
	if (rammb <= 8)
		set_swap(diskdev, NULL, 1);
	return 1;
}

/*
 * any additional partition validation
 */
int
md_check_partitions(void)
{
	return 1;
}


/* Upgrade support */
int
md_update(void)
{
	move_aout_libs();
	endwin();
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

	if (read_mbr(diskdev, &mbr, sizeof mbr) < 0)
		return;

	mbrp = &mbr.mbr_parts[0];

	for (i = 0; i < NMBRPART; i++) {
		if (mbrp[i].mbrp_typ == MBR_PTYPE_386BSD) {
			oldbsdpart = i;
			oldbsdcount++;
		} else if (mbrp[i].mbrp_typ == MBR_PTYPE_NETBSD)
			netbsdpart = i;
	}

	if (netbsdpart == -1 && oldbsdcount == 1) {
		mbrp[oldbsdpart].mbrp_typ = MBR_PTYPE_NETBSD;
		write_mbr(diskdev, &mbr, sizeof mbr, 0);
	}
}



void
md_cleanup_install(void)
{
	char realfrom[STRSIZE];
	char realto[STRSIZE];
	char cmd[STRSIZE];

	strncpy(realfrom, target_expand("/etc/rc.conf"), STRSIZE);
	strncpy(realto, target_expand("/etc/rc.conf.install"), STRSIZE);
	snprintf(cmd, sizeof cmd, "sed "
			"-e 's/rc_configured=NO/rc_configured=YES/' "
			" < %s > %s", realfrom, realto);
	scripting_fprintf(logfp, "%s\n", cmd);
	do_system(cmd);

	run_prog(RUN_FATAL, NULL, "mv -f %s %s", realto, realfrom);
	
	add_rc_conf("wscons=YES\n");

#if defined(__i386__)
	/*
	 * For GENERIC_TINY, do not enable any extra screens or wsmux.
	 * Otherwise, run getty on 4 VTs.
	 */
	if (strcmp(get_bootmodel(), "tiny") == 0) {
		strncpy(realfrom, target_expand("/etc/wscons.conf"), STRSIZE);
		strncpy(realto, target_expand("/etc/wscons.conf.install"),
		    STRSIZE);
		snprintf(cmd, sizeof cmd, "sed"
			    " -e '/^screen/s/^/#/'"
			    " -e '/^mux/s/^/#/'"
			    " < %s > %s", realfrom, realto);
	} else
#endif
	       {
		strncpy(realfrom, target_expand("/etc/ttys"), STRSIZE);
		strncpy(realto, target_expand("/etc/ttys.install"), STRSIZE);
		snprintf(cmd, sizeof cmd, "sed "
				"-e '/^ttyE[1-9]/s/off/on/'"
				" < %s > %s", realfrom, realto);
	}
		
	scripting_fprintf(logfp, "%s\n", cmd);
	do_system(cmd);
	run_prog(RUN_FATAL, NULL, "mv -f %s %s", realto, realfrom);

	run_prog(0, NULL, "rm -f %s", target_expand("/sysinst"));
	run_prog(0, NULL, "rm -f %s", target_expand("/.termcap"));
	run_prog(0, NULL, "rm -f %s", target_expand("/.profile"));
}

int
md_bios_info(dev)
	char *dev;
{
	int mib[2], i;
	size_t len;
	struct biosdisk_info *bip;
	struct nativedisk_info *nip = NULL, *nat;
	int cyl, head, sec;

	if (disklist == NULL) {
		mib[0] = CTL_MACHDEP;
		mib[1] = CPU_DISKINFO;
		if (sysctl(mib, 2, NULL, &len, NULL, 0) < 0)
			goto nogeom;
		disklist = malloc(len);
		if (disklist == NULL) {
			fprintf(stderr, "Out of memory\n");
			return -1;
		}
		sysctl(mib, 2, disklist, &len, NULL, 0);
	}

	nativedisk = NULL;

	for (i = 0; i < disklist->dl_nnativedisks; i++) {
		nat = &disklist->dl_nativedisks[i];
		if (!strcmp(dev, nat->ni_devname)) {
			nativedisk = nip = nat;
			break;
		}
	}
	if (nip == NULL || nip->ni_nmatches == 0) {
nogeom:
		msg_display(MSG_nobiosgeom, dlcyl, dlhead, dlsec);
		if (guess_biosgeom_from_mbr(&mbr, &cyl, &head, &sec) >= 0) {
			msg_display_add(MSG_biosguess, cyl, head, sec);
			set_bios_geom(cyl, head, sec);
		} else
			set_bios_geom(dlcyl, dlhead, dlsec);
		biosdisk = NULL;
	} else if (nip->ni_nmatches == 1) {
		bip = &disklist->dl_biosdisks[nip->ni_biosmatches[0]];
		msg_display(MSG_onebiosmatch);
		msg_table_add(MSG_onebiosmatch_header);
		msg_table_add(MSG_onebiosmatch_row, bip->bi_dev - 0x80,
		    bip->bi_cyl, bip->bi_head, bip->bi_sec);
		msg_display_add(MSG_biosgeom_advise);
		process_menu(MENU_biosonematch, NULL);
	} else {
		msg_display(MSG_biosmultmatch);
		msg_table_add(MSG_biosmultmatch_header);
		for (i = 0; i < nip->ni_nmatches; i++) {
			bip = &disklist->dl_biosdisks[nip->ni_biosmatches[i]];
			msg_table_add(MSG_biosmultmatch_row, i,
			    bip->bi_dev - 0x80, bip->bi_cyl, bip->bi_head,
			    bip->bi_sec);
		}
		process_menu(MENU_biosmultmatch, NULL);
	}
	if (biosdisk != NULL && (biosdisk->bi_flags & BIFLAG_EXTINT13))
		bsize = dlsize;
	else
		bsize = bcyl * bhead * bsec;
	bcylsize = bhead * bsec;
	return 0;
}

static int
count_mbr_parts(pt)
	struct mbr_partition *pt;
{
	int i, count = 0;

	for (i = 0; i < NMBRPART; i++)
		if (pt[i].mbrp_typ != 0)
			count++;

	return count;
}

static int
mbr_part_above_chs(mbr_partition_t *pt)
{
	return ((pt[bsdpart].mbrp_start + pt[bsdpart].mbrp_size) >=
		bcyl * bhead * bsec);
}

static int
mbr_partstart_above_chs(mbr_partition_t *pt)
{
	return (pt[bsdpart].mbrp_start >= bcyl * bhead * bsec);
}

#if defined(__i386__)
char *
get_bootmodel(void)
{
	struct utsname ut;
#ifdef DEBUG
	char *envstr;

	envstr = getenv("BOOTMODEL");
	if (envstr != NULL)
		return envstr;
#endif

	if (uname(&ut) < 0)
		return "";

	if (strstr(ut.version, "TINY") != NULL)
		return "tiny";
	else if (strstr(ut.version, "SMALL") != NULL)
		return "small";
	else if (strstr(ut.version, "LAPTOP") != NULL)
		return "laptop";
	else if (strstr(ut.version, "PS2") != NULL)
		return "ps2";
	return "";
}
#endif

void
md_init(void)
{
}

void
md_set_sizemultname(void)
{

	set_sizemultname_meg();
}

void
md_set_no_x(void)
{

	toggle_getit (8);
	toggle_getit (9);
	toggle_getit (10);
	toggle_getit (11);
	toggle_getit (12);
	toggle_getit (13);
}

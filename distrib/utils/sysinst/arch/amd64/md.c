/*	$NetBSD: md.c,v 1.4 2003/05/07 19:02:53 dsl Exp $ */

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

/* md.c -- Machine specific code for amd64 */

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


mbr_sector_t mbr;
int mbr_len;
int c1024_resp;
struct disklist *disklist = NULL;
struct nativedisk_info *nativedisk;
struct biosdisk_info *biosdisk = NULL;
int netbsd_mbr_installed = 0;
int netbsd_bootsel_installed = 0;

struct mbr_bootsel *mbs;
int defbootselpart, defbootseldisk;

/* prototypes */

static int md_read_bootcode(const char *, mbr_sector_t *);
static int count_mbr_parts(struct mbr_partition *);
static int mbr_part_above_chs(struct mbr_partition *);
static int mbr_partstart_above_chs(struct mbr_partition *);
static void configure_bootsel(void);
static void md_upgrade_mbrtype(void);


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
		process_menu(MENU_noyes);
		if (!yesno)
			goto edit;
	}

	if (count_mbr_parts(part) > 1) {
		msg_display(MSG_installbootsel);
		process_menu(MENU_yesno);
		if (yesno) {
			mbr_len = md_read_bootcode(_PATH_BOOTSEL, &mbr);
			configure_bootsel();
			netbsd_mbr_installed = netbsd_bootsel_installed = 1;
		} else {
			msg_display(MSG_installnormalmbr);
			process_menu(MENU_yesno);
			if (yesno) {
				mbr_len = md_read_bootcode(_PATH_MBR, &mbr);
				netbsd_mbr_installed = 1;
			}
		}
	} else {
		mbr_len = md_read_bootcode(_PATH_MBR, &mbr);
		netbsd_mbr_installed = 1;
	}

	if (mbr_partstart_above_chs(part) && !netbsd_mbr_installed) {
		msg_display(MSG_installmbr);
		process_menu(MENU_yesno);
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

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;

	if (fstat(fd, &st) < 0 || st.st_size != sizeof *mbr) {
		close(fd);
		return -1;
	}

	if (mbr->mbr_bootsel.mbrb_magic != native_to_le16(MBR_MAGIC))
		len = offsetof(mbr_sector_t, mbr_parts);
	else
		len = offsetof(mbr_sector_t, mbr_bootsel);

	if (read(fd, mbr, len) != len) {
		close(fd);
		return -1;
	}
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
		process_menu(MENU_ok);
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
		process_menu(MENU_getboottype);
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
	FILE *f;
	int i;
	int maxpart = getmaxpartitions();
	struct disklabel l;

editlab:
	/* Ask for layout type -- standard or special */
	msg_display(MSG_layout,
			(1.0*fsptsize*sectorsize)/MEG,
			(1.0*minfsdmb*sectorsize)/MEG,
			(1.0*minfsdmb*sectorsize)/MEG+rammb+XNEEDMB);
	process_menu(MENU_layout);

	if (layoutkind == 3) {
		ask_sizemult(dlcylsize);
	} else {
		sizemult = MEG / sectorsize;
		multname = msg_string(MSG_megname);
	}


	/* Build standard partitions */
	emptylabel(bsdlabel);

	/* Partitions C and D are predefined. */
	bsdlabel[C].pi_fstype = FS_UNUSED;
	bsdlabel[C].pi_offset = ptstart;
	bsdlabel[C].pi_size = fsptsize;
	
	bsdlabel[D].pi_fstype = FS_UNUSED;
	bsdlabel[D].pi_offset = 0;
	bsdlabel[D].pi_size = fsdsize;

	/* Standard fstypes */
	bsdlabel[A].pi_fstype = FS_BSDFFS;
	bsdlabel[A].pi_bsize  = 8192;
	bsdlabel[A].pi_fsize  = 1024;
	bsdlabel[B].pi_fstype = FS_SWAP;
	bsdlabel[E].pi_fstype = FS_UNUSED;
	bsdlabel[F].pi_fstype = FS_UNUSED;
	bsdlabel[G].pi_fstype = FS_UNUSED;
	bsdlabel[H].pi_fstype = FS_UNUSED;
	bsdlabel[I].pi_fstype = FS_UNUSED;
	bsdlabel[J].pi_fstype = FS_UNUSED;
	bsdlabel[K].pi_fstype = FS_UNUSED;
	bsdlabel[L].pi_fstype = FS_UNUSED;
	bsdlabel[M].pi_fstype = FS_UNUSED;
	bsdlabel[N].pi_fstype = FS_UNUSED;
	bsdlabel[O].pi_fstype = FS_UNUSED;
	bsdlabel[P].pi_fstype = FS_UNUSED;

	switch (layoutkind) {
	case 1: /* standard: a root, b swap, c/d "unused", e /usr */
	case 2: /* standard X: a root, b swap (big), c/d "unused", e /usr */
		partstart = ptstart;

		/* check that we have enough space */
		i = NUMSEC(20+2*rammb, MEG/sectorsize, dlcylsize);
		i += NUMSEC(layoutkind * 2 * (rammb < 16 ? 16 : rammb),
			   MEG/sectorsize, dlcylsize);
		if ( i > fsptsize) {
			msg_display(MSG_disktoosmall);
			process_menu(MENU_ok);
			goto custom;
		}
		/* Root */
		i = NUMSEC(20+2*rammb, MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC (i/(MEG/sectorsize)+1, MEG/sectorsize,
				   dlcylsize) - partstart;
		bsdlabel[A].pi_offset = partstart;
		bsdlabel[A].pi_size = partsize;
		strcpy (fsmount[A], "/");
		partstart += partsize;

		/* swap */
		i = NUMSEC(layoutkind * 2 * (rammb < 16 ? 16 : rammb),
			   MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC (i/(MEG/sectorsize)+1, MEG/sectorsize,
			   dlcylsize) - partstart;
		bsdlabel[B].pi_offset = partstart;
		bsdlabel[B].pi_size = partsize;
		partstart += partsize;

		/* /usr */
		partsize = fsptsize - (partstart - ptstart);
		bsdlabel[E].pi_fstype = FS_BSDFFS;
		bsdlabel[E].pi_offset = partstart;
		bsdlabel[E].pi_size = partsize;
		bsdlabel[E].pi_bsize = 8192;
		bsdlabel[E].pi_fsize = 1024;
		strcpy (fsmount[E], "/usr");
		break;

	case 3:
custom:

	case 4: /* use existing parts */
		if (get_real_geom(diskdev, &l) == 0) {
			msg_display(MSG_abort);
			return 0;
		}

#define p l.d_partitions[i]
		for (i = 0; i < maxpart; i++) {
			/*
			 * Make sure to not overwrite the NetBSD partition
			 * with blank values. Might happen when MBR doesn't
			 * contain any NetBSD entry yet, and the disklabel
			 * is fabricated by kernel.
			 */
			if (i == 2 && p.p_size == 0)
				continue;

			bsdlabel[i].pi_size = p.p_size;
			bsdlabel[i].pi_offset = p.p_offset;
			bsdlabel[i].pi_fstype = p.p_fstype;
			bsdlabel[i].pi_bsize  = p.p_fsize * p.p_frag;
			bsdlabel[i].pi_fsize  = p.p_fsize;
		}
#undef p

		msg_display(MSG_postuseexisting);
		getchar();
		break;
	}

	/*
	 * OK, we have a partition table. Give the user the chance to
	 * edit it and verify it's OK, or abort altogether.
	 */
	if (edit_and_check_label(bsdlabel, maxpart, RAW_PART, RAW_PART) == 0) {
		msg_display(MSG_abort);
		return 0;
	}

	if ((bsdlabel[A].pi_offset + bsdlabel[A].pi_size) / bcylsize > 1024 &&
	    (biosdisk == NULL || !(biosdisk->bi_flags & BIFLAG_EXTINT13))) {
		process_menu(MENU_cyl1024);
		/* XXX UGH! need arguments to process_menu */
		switch (c1024_resp) {
		case 1:
			edit_mbr(&mbr);
			/*FALLTHROUGH*/
		case 2:
			goto editlab;
		default:
			break;
		}
	}

	/* Disk name */
	msg_prompt (MSG_packname, "mydisk", bsddiskname, DISKNAME_SIZE);

	/* Create the disktab.preinstall */
#ifdef DEBUG
	f = fopen ("/tmp/disktab", "a");
#else
	f = fopen ("/etc/disktab", "w");
#endif
	if (f == NULL) {
		endwin();
		(void) fprintf (stderr, "Could not open /etc/disktab");
		exit (1);
	}
	(void)fprintf (f, "%s|NetBSD installation generated:\\\n", bsddiskname);
	(void)fprintf (f, "\t:dt=%s:ty=winchester:\\\n", disktype);
	(void)fprintf (f, "\t:nc#%d:nt#%d:ns#%d:\\\n", dlcyl, dlhead, dlsec);
	(void)fprintf (f, "\t:sc#%d:su#%d:\\\n", dlhead*dlsec, dlsize);
	(void)fprintf (f, "\t:se#%d:%s\\\n", sectorsize, doessf);
	for (i=0; i<maxpart; i++) {
		(void)fprintf (f, "\t:p%c#%d:o%c#%d:t%c=%s:",
			       'a'+i, bsdlabel[i].pi_size,
			       'a'+i, bsdlabel[i].pi_offset,
			       'a'+i, fstypenames[bsdlabel[i].pi_fstype]);
		if (bsdlabel[i].pi_fstype == FS_BSDFFS)
			(void)fprintf (f, "b%c#%d:f%c#%d",
				       'a'+i, bsdlabel[i].pi_bsize,
				       'a'+i, bsdlabel[i].pi_fsize);
		if (i < maxpart -1)
			(void)fprintf (f, "\\\n");
		else
			(void)fprintf (f, "\n");
	}
	fclose (f);

	/* Everything looks OK. */
	return (1);
}

int
md_pre_update(void)
{
	if (rammb <= 8)
		set_swap(diskdev, NULL, 1);
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
	sprintf(cmd, "sed "
			"-e 's/rc_configured=NO/rc_configured=YES/' "
			" < %s > %s", realfrom, realto);
	scripting_fprintf(logfp, "%s\n", cmd);
	do_system(cmd);

	run_prog(RUN_FATAL, NULL, "mv -f %s %s", realto, realfrom);
	
	add_rc_conf("wscons=YES\n");

	strncpy(realfrom, target_expand("/etc/ttys"), STRSIZE);
	strncpy(realto, target_expand("/etc/ttys.install"), STRSIZE);
	sprintf(cmd, "sed "
			"-e '/^ttyE[1-9]/s/off/on/'"
			" < %s > %s", realfrom, realto);

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
		process_menu(MENU_biosonematch);
	} else {
		msg_display(MSG_biosmultmatch);
		msg_table_add(MSG_biosmultmatch_header);
		for (i = 0; i < nip->ni_nmatches; i++) {
			bip = &disklist->dl_biosdisks[nip->ni_biosmatches[i]];
			msg_table_add(MSG_biosmultmatch_row, i,
			    bip->bi_dev - 0x80, bip->bi_cyl, bip->bi_head,
			    bip->bi_sec);
		}
		process_menu(MENU_biosmultmatch);
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

static void
configure_bootsel(void)
{
	struct mbr_partition *parts = &mbr.mbr_parts[0];
	int i;

	mbs = &mbr.mbr_bootsel;
	mbs->mbrb_flags = BFL_SELACTIVE;

	/* Setup default labels for partitions, since if not done by user */
	/* they don't get set and and bootselector doesn't 'appear' when  */
	/* it's loaded.                                                   */
	for (i = 0; i < NMBRPART; i++) {
		if (parts[i].mbrp_typ != 0 && mbs->mbrb_nametab[i][0] == '\0')
			snprintf(mbs->mbrb_nametab[i], sizeof(mbs->mbrb_nametab[0]),
			    "entry %d", i);
	}

	process_menu(MENU_configbootsel);

	for (i = 0; i < NMBRPART; i++) {
		if (parts[i].mbrp_typ != 0 &&
		   parts[i].mbrp_start >= (bcyl * bhead * bsec)) {
			mbs->mbrb_flags |= BFL_EXTINT13;
			break;
		}
	}
}

void
md_init(void)
{
}

#ifdef notdef
void
md_set_sizemultname(void)
{

	set_sizemultname_meg();
}
#endif

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

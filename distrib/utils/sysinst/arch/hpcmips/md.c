/*	$NetBSD: md.c,v 1.13 2002/06/14 03:29:28 lukem Exp $ */

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

#include <stdio.h>
#include <util.h>
#include <sys/param.h>
#include <machine/cpu.h>
#include <sys/sysctl.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"
#if !defined(__i386__)
#include "endian.h"
#include "mbr.h"
#endif


#ifdef __i386__
char mbr[512];
#else
extern char mbr[512];
#endif
int mbr_present, mbr_len;
int c1024_resp;
struct disklist *disklist = NULL;
struct nativedisk_info *nativedisk;
struct biosdisk_info *biosdisk = NULL;
int netbsd_mbr_installed = 0;
int netbsd_bootsel_installed = 0;

#ifdef __i386__
static int md_read_bootcode (char *, char *, size_t);
static int count_mbr_parts (struct mbr_partition *);
static int mbr_part_above_chs (struct mbr_partition *);
static int mbr_partstart_above_chs (struct mbr_partition *);
static void configure_bootsel (void);
#endif
static void md_upgrade_mbrtype (void);

struct mbr_bootsel *mbs;
int defbootselpart, defbootseldisk;


/* prototypes */


int
md_get_info()
{
	read_mbr(diskdev, mbr, sizeof mbr);
	if (!valid_mbr(mbr)) {
		memset(&mbr[MBR_PARTOFF], 0,
		    NMBRPART * sizeof (struct mbr_partition));
		/* XXX check result and give up if < 0 */
#ifdef __i386__
		mbr_len = md_read_bootcode(_PATH_MBR, mbr, sizeof mbr);
#else
		*((u_int16_t *)&mbr[MBR_MAGICOFF]) = le_to_native16(MBR_MAGIC);
#endif
		netbsd_mbr_installed = 1;
	} else
		mbr_len = MBR_SECSIZE;
	md_bios_info(diskdev);

#ifdef __i386__
edit:
#endif
	edit_mbr((struct mbr_partition *)&mbr[MBR_PARTOFF]);

#ifdef __i386__
	if (mbr_part_above_chs(part) &&
	    (biosdisk == NULL || !(biosdisk->bi_flags & BIFLAG_EXTINT13))) {
		msg_display(MSG_partabovechs);
		process_menu(MENU_noyes);
		if (!yesno)
			goto edit;
	}
#endif

#ifdef __i386__
	if (count_mbr_parts(part) > 1) {
		msg_display(MSG_installbootsel);
		process_menu(MENU_yesno);
		if (yesno) {
			mbr_len =
			    md_read_bootcode(_PATH_BOOTSEL, mbr, sizeof mbr);
			configure_bootsel();
			netbsd_mbr_installed = netbsd_bootsel_installed = 1;
		}
	}

	if (mbr_partstart_above_chs(part) && !netbsd_mbr_installed) {
		msg_display(MSG_installmbr);
		process_menu(MENU_yesno);
		if (yesno) {
			mbr_len = md_read_bootcode(_PATH_MBR, mbr, sizeof mbr);
			netbsd_mbr_installed = 1;
		}
	}

#endif
	/* Compute minimum NetBSD partition sizes (in sectors). */
	minfsdmb = STDNEEDMB * (MEG / sectorsize);

	return 1;
}

#ifdef __i386__
/*
 * Read MBR code from a file. It may be a maximum of "len" bytes
 * long. This function skips the partition table. Space for this
 * is assumed to be in the file, but a table already in the buffer
 * is not overwritten.
 */
static int
md_read_bootcode(path, buf, len)
	char *path, *buf;
	size_t len;
{
	int fd, cc;
	struct stat st;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;

	if (fstat(fd, &st) < 0 || st.st_size > len || st.st_size < MBR_SECSIZE){
		close(fd);
		return -1;
	}
	if (read(fd, buf, MBR_PARTOFF) != MBR_PARTOFF) {
		close(fd);
		return -1;
	}
	if (lseek(fd, MBR_MAGICOFF, SEEK_SET) < 0) {
		close(fd);
		return -1;
	}
	cc = read(fd, &buf[MBR_MAGICOFF], st.st_size - MBR_MAGICOFF);

	close(fd);

	return (cc + MBR_MAGICOFF);
}
#endif

int
md_pre_disklabel()
{
	msg_display(MSG_dofdisk);

	/* write edited MBR onto disk. */
	if (write_mbr(diskdev, mbr, sizeof mbr, 1) != 0) {
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
	/* Sector forwarding / badblocks ... */
	if (*doessf) {
		msg_display(MSG_dobad144);
		return run_prog(RUN_DISPLAY, NULL, "/usr/sbin/bad144 %s 0",
		    diskdev);
	}
	return 0;
}

int
md_post_newfs(void)
{
#if !defined(__i386__)
	return 0;
#else
	/* boot blocks ... */
	msg_display(MSG_dobootblks, diskdev);
	return run_prog(RUN_DISPLAY, NULL,
	    "/usr/mdec/installboot -v /usr/mdec/biosboot.sym /dev/r%sa",
	    diskdev);
#endif
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
	int part;
	int maxpart = getmaxpartitions();
	int remain;

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
	bsdlabel[B].pi_fstype = FS_SWAP;
	bsdlabel[E].pi_fstype = FS_UNUSED;
	bsdlabel[F].pi_fstype = FS_UNUSED;
	bsdlabel[G].pi_fstype = FS_UNUSED;
	bsdlabel[H].pi_fstype = FS_UNUSED;

	switch (layoutkind) {
	case 1: /* standard: a root, b swap, c/d "unused", e /usr */
	case 2: /* standard X: a root, b swap (big), c/d "unused", e /usr */
		partstart = ptstart;

		/* check that we have enouth space */
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
		bsdlabel[A].pi_bsize = 8192;
		bsdlabel[A].pi_fsize = 1024;
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

	case 3: /* custom: ask user for all sizes */
custom:		ask_sizemult(dlcylsize);
		msg_display(MSG_defaultunit, multname);
		partstart = ptstart;
		remain = fsptsize;

		/* root */
		i = NUMSEC(20+2*rammb, MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC (i/(MEG/sectorsize)+1, MEG/sectorsize,
				   dlcylsize) - partstart;
		if (partsize > remain)
			partsize = remain;
		msg_display_add(MSG_askfsroot1, remain/sizemult, multname);
		partsize = getpartsize(MSG_askfsroot2, partstart, partsize);
		bsdlabel[A].pi_offset = partstart;
		bsdlabel[A].pi_size = partsize;
		bsdlabel[A].pi_bsize = 8192;
		bsdlabel[A].pi_fsize = 1024;
		strcpy (fsmount[A], "/");
		partstart += partsize;
		remain -= partsize;
		
		/* swap */
		i = NUMSEC( 2 * (rammb < 16 ? 16 : rammb),
			   MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC (i/(MEG/sectorsize)+1, MEG/sectorsize,
			   dlcylsize) - partstart;
		if (partsize > remain)
			partsize = remain;
		msg_display(MSG_askfsswap1, remain/sizemult, multname);
		partsize = getpartsize(MSG_askfsswap2, partstart, partsize);
		bsdlabel[B].pi_offset = partstart;
		bsdlabel[B].pi_size = partsize;
		partstart += partsize;
		remain -= partsize;
		
		/* Others E, F, G, H */
		part = E;
		if (remain > 0)
			msg_display (MSG_otherparts);
		while (remain > 0 && part <= H) {
			msg_display_add(MSG_askfspart1, diskdev,
			    partition_name(part), remain/sizemult, multname);
			partsize = getpartsize(MSG_askfspart2, partstart,
			    remain);
			if (partsize > 0) {
				if (remain - partsize < sizemult)
					partsize = remain;
				bsdlabel[part].pi_fstype = FS_BSDFFS;
				bsdlabel[part].pi_offset = partstart;
				bsdlabel[part].pi_size = partsize;
				bsdlabel[part].pi_bsize = 8192;
				bsdlabel[part].pi_fsize = 1024;
				if (part == E)
					strcpy (fsmount[E], "/usr");
				msg_prompt_add (MSG_mountpoint, fsmount[part],
						fsmount[part], 20);
				partstart += partsize;
				remain -= partsize;
			}
			part++;
		}
		
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

#ifdef __i386__
	/*
	 * XXX check for int13 extensions.
	 */
	if ((bsdlabel[A].pi_offset + bsdlabel[A].pi_size) / bcylsize > 1024 &&
	    (biosdisk == NULL || !(biosdisk->bi_flags & BIFLAG_EXTINT13))) {
		process_menu(MENU_cyl1024);
		/* XXX UGH! need arguments to process_menu */
		switch (c1024_resp) {
		case 1:
			edit_mbr((struct mbr_partition *)&mbr[MBR_PARTOFF]);
			/*FALLTHROUGH*/
		case 2:
			goto editlab;
		default:
			break;
		}
	}
#endif

	/* Disk name */
	msg_prompt (MSG_packname, "mydisk", bsddiskname, DISKNAME_SIZE);

	/* Create the disktab.preinstall */
	run_prog (0, NULL, "cp /etc/disktab.preinstall /etc/disktab");
#ifdef DEBUG
	f = fopen ("/tmp/disktab", "a");
#else
	f = fopen ("/etc/disktab", "a");
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
	for (i=0; i<8; i++) {
		(void)fprintf (f, "\t:p%c#%d:o%c#%d:t%c=%s:",
			       'a'+i, bsdlabel[i].pi_size,
			       'a'+i, bsdlabel[i].pi_offset,
			       'a'+i, fstypenames[bsdlabel[i].pi_fstype]);
		if (bsdlabel[i].pi_fstype == FS_BSDFFS)
			(void)fprintf (f, "b%c#%d:f%c#%d",
				       'a'+i, bsdlabel[i].pi_bsize,
				       'a'+i, bsdlabel[i].pi_fsize);
		if (i < 7)
			(void)fprintf (f, "\\\n");
		else
			(void)fprintf (f, "\n");
	}
	fclose (f);

	/* Everything looks OK. */
	return (1);
}


/* Upgrade support */
int
md_update(void)
{
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
md_upgrade_mbrtype()
{
	struct mbr_partition *mbrp;
	int i, netbsdpart = -1, oldbsdpart = -1, oldbsdcount = 0;

	if (read_mbr(diskdev, mbr, sizeof mbr) < 0)
		return;

	mbrp = (struct mbr_partition *)&mbr[MBR_PARTOFF];

	for (i = 0; i < NMBRPART; i++) {
		if (mbrp[i].mbrp_typ == MBR_PTYPE_386BSD) {
			oldbsdpart = i;
			oldbsdcount++;
		} else if (mbrp[i].mbrp_typ == MBR_PTYPE_NETBSD)
			netbsdpart = i;
	}

	if (netbsdpart == -1 && oldbsdcount == 1) {
		mbrp[oldbsdpart].mbrp_typ = MBR_PTYPE_NETBSD;
		write_mbr(diskdev, mbr, sizeof mbr, 0);
	}
}



void
md_cleanup_install(void)
{
	char realfrom[STRSIZE];
	char realto[STRSIZE];
	char sedcmd[STRSIZE];

	strncpy(realfrom, target_expand("/etc/rc.conf"), STRSIZE);
	strncpy(realto, target_expand("/etc/rc.conf.install"), STRSIZE);

	sprintf(sedcmd, "sed 's/rc_configured=NO/rc_configured=YES/' < %s > %s",
	    realfrom, realto);
	scripting_fprintf(log, "%s\n", sedcmd);
	do_system(sedcmd);

	run_prog(RUN_FATAL, NULL, "mv -f %s %s", realto, realfrom);
	run_prog(0, NULL, "rm -f %s", target_expand("/sysinst"));
	run_prog(0, NULL, "rm -f %s", target_expand("/.termcap"));
	run_prog(0, NULL, "rm -f %s", target_expand("/.profile"));
}

int
md_bios_info(dev)
	char *dev;
{
#ifdef __i386__
	int mib[2], i, len;
	struct biosdisk_info *bip;
	struct nativedisk_info *nip = NULL, *nat;
#endif
	int cyl, head, sec;

#ifdef __i386__
	if (disklist == NULL) {
		mib[0] = CTL_MACHDEP;
		mib[1] = CPU_DISKINFO;
		if (sysctl(mib, 2, NULL, &len, NULL, 0) < 0)
			goto nogeom;
		disklist = (struct disklist *)malloc(len);
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
#endif
		msg_display(MSG_nobiosgeom, dlcyl, dlhead, dlsec);
		if (guess_biosgeom_from_mbr(mbr, &cyl, &head, &sec) >= 0) {
			msg_display_add(MSG_biosguess, cyl, head, sec);
			set_bios_geom(cyl, head, sec);
		} else
			set_bios_geom(dlcyl, dlhead, dlsec);
		biosdisk = NULL;
#ifdef __i386__
	} else if (nip->ni_nmatches == 1) {
		bip = &disklist->dl_biosdisks[nip->ni_biosmatches[0]];
		msg_display(MSG_onebiosmatch);
		msg_table_add(MSG_onebiosmatch_header);
		msg_table_add(MSG_onebiosmatch_row, bip->bi_dev - 0x80,
		    bip->bi_cyl, bip->bi_head, bip->bi_sec);
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
#endif
		bsize = bcyl * bhead * bsec;
	bcylsize = bhead * bsec;
	return 0;
}

#ifdef __i386__
static int
count_mbr_parts(pt)
	struct mbr_partition *pt;
{
	int i, count = 0;;

	for (i = 0; i < NMBRPART; i++)
		if (pt[i].mbrp_typ != 0)
			count++;

	return count;
}

static int
mbr_part_above_chs(pt)
	struct mbr_partition *pt;
{
	return ((pt[bsdpart].mbrp_start + pt[bsdpart].mbrp_size) >=
		bcyl * bhead * bsec);
}

static int
mbr_partstart_above_chs(pt)
	struct mbr_partition *pt;
{
	return (pt[bsdpart].mbrp_start >= bcyl * bhead * bsec);
}

static void
configure_bootsel()
{
	struct mbr_partition *parts =
	    (struct mbr_partition *)&mbr[MBR_PARTOFF];
	int i;


	mbs = (struct mbr_bootsel *)&mbr[MBR_BOOTSELOFF];
	mbs->flags = BFL_SELACTIVE;

	/* Setup default labels for partitions, since if not done by user */
	/* they don't get set and and bootselector doesn't 'appear' when  */
	/* it's loaded.                                                   */
	for (i = 0; i < NMBRPART; i++) {
		if (parts[i].mbrp_typ != 0 && mbs->nametab[i][0] == '\0')
			snprintf(mbs->nametab[i], sizeof(mbs->nametab[0]),
			    "entry %d", i+1);
	}

	process_menu(MENU_configbootsel);

	for (i = 0; i < NMBRPART; i++) {
		if (parts[i].mbrp_typ != 0 &&
		   parts[i].mbrp_start >= (bcyl * bhead * bsec)) {
			mbs->flags |= BFL_EXTINT13;
			break;
		}
	}
}

void
disp_bootsel(part, mbsp)
	struct mbr_partition *part;
	struct mbr_bootsel *mbsp;
{
	int i;

	msg_table_add(MSG_bootsel_header);
	for (i = 0; i < 4; i++) {
		msg_table_add(MSG_bootsel_row,
		    i, get_partname(i), mbs->nametab[i]);
	}
}
#endif

int
md_pre_update()
{
	return 1;
}

void
md_init()
{
}

void
md_set_sizemultname()
{

	set_sizemultname_meg();
}

void
md_set_no_x()
{

	toggle_getit (8);
	toggle_getit (9);
	toggle_getit (10);
	toggle_getit (11);
	toggle_getit (12);
	toggle_getit (13);
}

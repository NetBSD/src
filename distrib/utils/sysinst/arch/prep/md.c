/*	$NetBSD: md.c,v 1.7 2003/05/07 10:20:24 dsl Exp $	*/

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

/* md.c -- Machine specific code for prep */

#include <sys/param.h>
#include <sys/sysctl.h>

#include <stdio.h>
#include <util.h>

#include <machine/cpu.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"
#include "endian.h"

mbr_sector_t mbr;

int mbr_present, mbr_len;
int c1024_resp;
struct disklist *disklist = NULL;
struct nativedisk_info *nativedisk;
struct biosdisk_info *biosdisk = NULL;
int netbsd_mbr_installed = 0;


/* prototypes */


int
md_get_info(void)
{

	read_mbr(diskdev, &mbr, sizeof mbr);
	if (!valid_mbr(&mbr)) {
		memset(&mbr.mbr_parts, 0, sizeof mbr.mbr_parts);
		/* XXX check result and give up if < 0 */
		mbr.mbr_signature = le_to_native16(MBR_MAGIC);
		netbsd_mbr_installed = 1;
	} else
		mbr_len = MBR_SECSIZE;

	edit_mbr(&mbr);

	/* Compute minimum NetBSD partition sizes (in sectors). */
	minfsdmb = STDNEEDMB * (MEG / sectorsize);

	return 1;
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

	return 0;
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

	/* Partitions C is predefined. */
	bsdlabel[D].pi_fstype = FS_UNUSED;
	bsdlabel[D].pi_offset = 0;
	bsdlabel[D].pi_size = fsdsize;

	/* Standard fstypes */
	bsdlabel[A].pi_fstype = FS_BSDFFS;
	bsdlabel[A].pi_bsize  = 8192;
	bsdlabel[A].pi_fsize  = 1024;
	bsdlabel[B].pi_fstype = FS_SWAP;
	bsdlabel[D].pi_fstype = FS_UNUSED;
	bsdlabel[E].pi_fstype = FS_UNUSED;
	bsdlabel[F].pi_fstype = FS_UNUSED;
	bsdlabel[G].pi_fstype = FS_UNUSED;
	bsdlabel[H].pi_fstype = FS_UNUSED;

	switch (layoutkind) {
	case 1: /* standard: a root, b swap, c "unused", d /usr */
	case 2: /* standard X: a root, b swap (big), c "unused", d /usr */
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
		bsdlabel[D].pi_fstype = FS_BSDFFS;
		bsdlabel[D].pi_offset = partstart;
		bsdlabel[D].pi_size = partsize;
		bsdlabel[D].pi_bsize = 8192;
		bsdlabel[D].pi_fsize = 1024;
		strcpy (fsmount[D], "/usr");
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
		
		/* Others D, E, F, G, H */
		part = D;
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
				if (part == D)
					strcpy (fsmount[part], "/usr");
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
	return 1;
}

/* Upgrade support */
int
md_update(void)
{

	endwin();
	md_copy_filesystem();
	md_post_newfs();
	wrefresh(curscr);
	wmove(stdscr, 0, 0);
	wclear(stdscr);
	wrefresh(stdscr);
	return 1;
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
	scripting_fprintf(logfp, "%s\n", sedcmd);
	do_system(sedcmd);

	run_prog(RUN_FATAL, NULL, "mv -f %s %s", realto, realfrom);
	run_prog(0, NULL, "rm -f %s", target_expand("/sysinst"));
	run_prog(0, NULL, "rm -f %s", target_expand("/.termcap"));
	run_prog(0, NULL, "rm -f %s", target_expand("/.profile"));
}

int
md_pre_update(void)
{

	return 1;
}

void
md_init(void)
{

	/* Nothing to do */
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

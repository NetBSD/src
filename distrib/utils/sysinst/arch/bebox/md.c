/*	$NetBSD: md.c,v 1.34 2003/05/30 22:17:02 dsl Exp $ */

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

/* md.c -- Machine specific code for bebox */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <stdio.h>
#include <util.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"


int mbr_present;
int c1024_resp;
mbr_sector_t mbr;

/* prototypes */


int
md_get_info(void)
{

	read_mbr(diskdev, &mbr, sizeof mbr);
	md_bios_info(diskdev);
	return edit_mbr(&mbr);
}

int
md_pre_disklabel(void)
{
	printf ("%s", msg_string (MSG_dofdisk));

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
		printf ("%s", msg_string (MSG_dobad144));
		return run_prog(RUN_DISPLAY, NULL, "/usr/sbin/bad144 %s 0",
		    diskdev);
	}
	return 0;
}

int
md_post_newfs(void)
{
	/* boot blocks ... */
	printf (msg_string(MSG_dobootblks), diskdev);
	run_prog (RUN_DISPLAY, NULL,
	    "/usr/mdec/installboot -v /usr/mdec/biosboot.sym "
		  "/dev/r%sa", diskdev);
	return 0;
}

int
md_copy_filesystem(void)
{
	if (target_already_root()) {
		return 0;
	}

	/* Copy the instbin(s) to the disk */
	printf ("%s", msg_string(MSG_dotar));
	if (run_prog (RUN_DISPLAY, NULL, "pax -X -O -r -w -pe / /mnt") != 0)
		return 1;

	/* Copy next-stage install profile into target /.profile. */
	if (cp_to_target ("/tmp/.hdprofile", "/.profile")!= 0)
		return 1;
	return cp_to_target ("/usr/share/misc/termcap", "/.termcap");
}



int
md_make_bsd_partitions(void)
{
	return make_bsd_partitions();
}

int
md_check_partitions(void)
{
	return 1;
}

/* Upgrade support */
int
md_update(void)
{
	endwin();
	md_copy_filesystem ();
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
}

int
md_bios_info(char *dev)
{
	int cyl, head, sec;

	msg_display(MSG_nobiosgeom, dlcyl, dlhead, dlsec);
	if (guess_biosgeom_from_mbr(&mbr, &cyl, &head, &sec) >= 0) {
		msg_display_add(MSG_biosguess, cyl, head, sec);
		set_bios_geom(cyl, head, sec);
	} else
		set_bios_geom(dlcyl, dlhead, dlsec);
	bsize = bcyl * bhead * bsec;
	bcylsize = bhead * bsec;
	return 0;
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

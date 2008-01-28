/*	$NetBSD: md.c,v 1.21 2008/01/28 02:47:16 rumble Exp $ */

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

/* md.c -- Machine specific code for playstation2 */

#include <stdio.h>
#include <util.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

int
md_get_info()
{
	int cyl, head, sec;

	read_mbr(diskdev, &mbr);

	msg_display(MSG_nobiosgeom, dlcyl, dlhead, dlsec);

	if (guess_biosgeom_from_mbr(&mbr, &cyl, &head, &sec) >= 0)
		msg_display_add(MSG_biosguess, cyl, head, sec);
	set_bios_geom(cyl, head, sec);

	edit_mbr(&mbr);

	return (1);
}

int
md_pre_disklabel()
{

	msg_display(MSG_dofdisk);

	/* write edited MBR onto disk. */
	if (write_mbr(diskdev, &mbr, 1) != 0) {
		msg_display(MSG_wmbrfail);
		process_menu(MENU_ok, NULL);

		return (1);
	}

	return (0);
}

int
md_post_disklabel()
{
	/* Sector forwarding / badblocks ... */
	if (*doessf) {
		msg_display(MSG_dobad144);
		return (run_program(RUN_DISPLAY, "/usr/sbin/bad144 %s 0",
		    diskdev));
	}

	return (0);
}

int
md_post_newfs()
{

	return (0);
}

int
md_copy_filesystem()
{

	return (0);
}

int
md_make_bsd_partitions()
{

	return (make_bsd_partitions());
}

/* Upgrade support */
int
md_update()
{

	endwin();
	md_copy_filesystem();
	md_post_newfs();
	wrefresh(curscr);
	wmove(stdscr, 0, 0);
	wclear(stdscr);
	wrefresh(stdscr);

	return (1);
}

void
md_cleanup_install()
{

	enable_rc_conf();

	run_program(0, "rm -f %s", target_expand("/sysinst"));
	run_program(0, "rm -f %s", target_expand("/.termcap"));
	run_program(0, "rm -f %s", target_expand("/.profile"));
}

int
md_pre_update()
{

	return (1);
}

void
md_init()
{
	/* Nothing to do */
}

void
md_init_set_status(int minimal)
{
	(void)minimal;
}

int
md_check_partitions()
{

	return (1);
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

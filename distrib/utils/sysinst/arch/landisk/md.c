/*	$NetBSD: md.c,v 1.6 2009/05/16 10:40:17 nonaka Exp $	*/

/*
 * Copyright 1997,2002 Piermont Information Systems Inc.
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

/* md.c -- Machine specific code for landisk */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <stdio.h>
#include <util.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"


int
md_check_partitions(void)
{

	return 1;
}

void
md_cleanup_install(void)
{

	enable_rc_conf();
}

int
md_copy_filesystem(void)
{

	return 0;
}

int
md_get_info(void)
{

	read_mbr(diskdev, &mbr);
	md_bios_info(diskdev);
	return edit_mbr(&mbr);
}

int
md_make_bsd_partitions(void)
{

	return make_bsd_partitions();
}

int
md_post_disklabel(void)
{

	return 0;
}

/*
 * MD hook called after upgrade() or install() has finished setting
 * up the target disk but immediately before the user is given the
 * ``disks are now set up'' message, so that if power fails, they can
 * continue installation by booting the target disk and doing an
 * `upgrade'.
 *
 * On the landisk, we use this opportunity to install the boot blocks.
 */
int
md_post_newfs(void)
{
	char *bootxx;
	int error;

	printf (msg_string(MSG_dobootblks), diskdev);
	cp_to_target("/usr/mdec/boot", "/boot");
	bootxx = bootxx_name();
	if (bootxx != NULL) {
		error = run_program(RUN_DISPLAY | RUN_NO_CLEAR,
		    "/usr/sbin/installboot -v /dev/r%sa %s", diskdev, bootxx);
		free(bootxx);
	} else
		error = -1;

	if (error != 0)
		process_menu(MENU_ok,
		    deconst("Warning: disk is probably not bootable"));

	return 0;
}

int
md_pre_disklabel(void)
{

	if (no_mbr)
		return 0;

	msg_display(MSG_dofdisk);

	/* write edited MBR onto disk. */
	if (write_mbr(diskdev, &mbr, 1) != 0 ||
	    run_program(RUN_SILENT | RUN_ERROR_OK,
	    "/sbin/fdisk -f -i -c /usr/mdec/mbr %s", diskdev)) {
		msg_display(MSG_wmbrfail);
		process_menu(MENU_ok, NULL);
		return 1;
	}

	return 0;
}

int
md_pre_update(void)
{

	return 1;
}

int
md_update(void)
{

	md_copy_filesystem();
	md_post_newfs();
	wrefresh(curscr);
	wmove(stdscr, 0, 0);
	wclear(stdscr);
	wrefresh(stdscr);
	return 1;
}

void
md_init(void)
{

}

int
md_bios_info(dev)
	char *dev;
{
	int cyl, head;
	daddr_t sec;

	msg_display(MSG_nobiosgeom, dlcyl, dlhead, dlsec);
	if (guess_biosgeom_from_mbr(&mbr, &cyl, &head, &sec) >= 0)
		msg_display_add(MSG_biosguess, cyl, head, sec);
	set_bios_geom(cyl, head, sec);

	return 0;
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

void
md_init_set_status(int minimal)
{
	(void)minimal;
}

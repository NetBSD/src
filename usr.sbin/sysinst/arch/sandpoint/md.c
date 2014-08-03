/*	$NetBSD: md.c,v 1.2 2014/08/03 16:09:40 martin Exp $ */

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

/* md.c -- sandpoint machine specific routines */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>

#include <stdio.h>
#include <string.h>
#include <util.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

static char *prodname;

void
md_init(void)
{
}

void
md_init_set_status(int flags)
{
	static const char mib_name[] = "machdep.prodfamily";
	static char unknown[] = "unknown";
	size_t len;

	(void)flags;

	/*
	 * Determine the product family of the board we are running on and
	 * enable the installation of the corresponding GENERIC kernel.
	 *
	 * Note:  In md.h the two kernels are disabled.  If they are
	 *        enabled there the logic here needs to be switched.
	 */
	if (sysctlbyname(mib_name, NULL, &len, NULL, 0) != 0) {
		prodname = unknown;
		return;
	}
	prodname = malloc(len);
	sysctlbyname(mib_name, prodname, &len, NULL, 0);

	if (strcmp(prodname, "kurobox")==0 || strcmp(prodname, "kurot4")==0)
		/*
		 * Running on a KuroBox family product, so enable KUROBOX
		 */
		set_kernel_set(SET_KERNEL_2);
        else
		/*
		 * Otherwise enable GENERIC
		 */
		set_kernel_set(SET_KERNEL_1);
}

int
md_get_info(void)
{
	return set_bios_geom_with_mbr_guess();
}

/*
 * md back-end code for menu-driven BSD disklabel editor.
 */
int
md_make_bsd_partitions(void)
{
	return make_bsd_partitions();
}

/*
 * any additional partition validation
 */
int
md_check_partitions(void)
{
	return 1;
}

/*
 * hook called before writing new disklabel.
 */
int
md_pre_disklabel(void)
{
	printf ("%s", msg_string (MSG_dofdisk));

	/* write edited MBR onto disk. */
	if (write_mbr(pm->diskdev, &mbr, 1) != 0) {
		msg_display(MSG_wmbrfail);
		process_menu(MENU_ok, NULL);
		return 1;
	}
	return 0;
}

/*
 * hook called after writing disklabel to new target disk.
 */
int
md_post_disklabel(void)
{
	/* Sector forwarding / badblocks ... */
	if (*pm->doessf) {
		printf ("%s", msg_string (MSG_dobad144));
		return run_program(RUN_DISPLAY, "/usr/sbin/bad144 %s 0",
		    pm->diskdev);
	}
	return 0;
}

/*
 * hook called after upgrade() or install() has finished setting
 * up the target disk but immediately before the user is given the
 * ``disks are now set up'' message.
 */
int
md_post_newfs(void)
{
	/* no boot blocks, we are using altboot */
	return 0;
}

int
md_post_extract(void)
{
	return 0;
}

void
md_cleanup_install(void)
{
#ifndef DEBUG
	int new_speed;

	enable_rc_conf();

	/*
	 * Set the console speed in /etc/ttys depending on the board.
	 * The default speed is 115200, which is patched when needed.
	 */
	if (strcmp(prodname, "kurobox")==0 || strcmp(prodname, "kurot4")==0)
		new_speed = 57600;			/* KuroBox */

	else if (strcmp(prodname, "dlink") == 0 ||	/* D-Link DSM-G600 */
	    strcmp(prodname, "nhnas") == 0)		/* NH23x, All6250 */
		new_speed = 9600;

	else
		new_speed = 0;

	if (new_speed != 0) {
		run_program(RUN_CHROOT, "sed -an -e 's/115200/%d/;H;$!d;g;w"
		    "/etc/ttys' /etc/ttys", new_speed);
	}
#endif
}

int
md_pre_update(void)
{
	return 1;
}

/* Upgrade support */
int
md_update(void)
{
	md_post_newfs();
	return 1;
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

int
md_pre_mount()
{
	return 0;
}

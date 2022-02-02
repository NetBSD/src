/*	$NetBSD: md.c,v 1.6.2.4 2022/02/02 04:25:36 msaitoh Exp $ */

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

/* md.c -- alpha machine specific routines */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <stdio.h>
#include <curses.h>
#include <unistd.h>
#include <fcntl.h>
#include <util.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

void
md_init(void)
{
}

void
md_init_set_status(int flags)
{
	(void)flags;
}

bool
md_get_info(struct install_partition_desc *install)
{
	struct disklabel disklabel;
	int fd;
	char dev_name[100];

	snprintf (dev_name, 100, "/dev/r%s%c", pm->diskdev, 'a' + getrawpartition());

	fd = open (dev_name, O_RDONLY, 0);
	if (fd < 0) {
		endwin();
		fprintf (stderr, "Can't open %s\n", dev_name);
		exit(1);
	}
	if (ioctl(fd, DIOCGDINFO, &disklabel) == -1) {
		endwin();
		fprintf (stderr, "Can't read disklabel on %s.\n", dev_name);
		close(fd);
		exit(1);
	}
	close(fd);

	pm->dlcyl = disklabel.d_ncylinders;
	pm->dlhead = disklabel.d_ntracks;
	pm->sectorsize = disklabel.d_secsize;
	pm->dlsize = disklabel.d_secperunit;

	/*
	 * Tru64 UNIX's disklabel is the same format as BSD disklabel,
	 * and it seems Tru64 stores incorrect geometry values in
	 * d_nsectors (sectors/track) and d_secpercyl (sectors/cylinder).
	 * d_secperunit seems always reliable so use it to get
	 * dlsec (sectors/track) and dlcylsize (sectors/cylinder) values.
	 * See PR/48697 for details.
	 */
	pm->dlsec = pm->dlsize / (pm->dlhead * pm->dlcyl);
	pm->dlcylsize = pm->dlsec * pm->dlhead;

	return true;
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
	return 1;
}

/*
 * hook called before writing new disklabel.
 */
bool
md_pre_disklabel(struct install_partition_desc *install,
    struct disk_partitions *part)
{
	return true;
}

/*
 * hook called after writing disklabel to new target disk.
 */
bool
md_post_disklabel(struct install_partition_desc *install,
    struct disk_partitions *part)
{
	return true;
}

/*
 * hook called after upgrade() or install() has finished setting
 * up the target disk but immediately before the user is given the
 * ``disks are now set up'' message.
 *
 * On the Alpha, we use this opportunity to install the boot blocks.
 */
int
md_post_newfs(struct install_partition_desc *install)
{
	char *bootxx;
	int error;

	msg_fmt_display(MSG_dobootblks, "%s", pm->diskdev);
	cp_to_target("/usr/mdec/boot", "/boot");
	bootxx = bootxx_name(install);
	if (bootxx != NULL) {
		error = run_program(RUN_DISPLAY | RUN_NO_CLEAR,
		    "/usr/sbin/installboot /dev/r%sc %s", pm->diskdev, bootxx);
		free(bootxx);
	} else
		error = -1;

	if (error != 0)
		process_menu(MENU_ok,
		    __UNCONST("Warning: disk is probably not bootable"));

	wclear(stdscr);
	touchwin(stdscr);
	clearok(stdscr, 1);
	refresh();

	return 0;
}

int
md_post_extract(struct install_partition_desc *install, bool upgrade)
{
	return 0;
}

void
md_cleanup_install(struct install_partition_desc *install)
{
#ifndef DEBUG
	enable_rc_conf();
#endif
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
	md_post_newfs(install);
	return 1;
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

	return true;
}
#endif

bool
md_parts_use_wholedisk(struct disk_partitions *parts)
{
	return parts_use_wholedisk(parts, 0, NULL);
}


/*	$NetBSD: md.c,v 1.23.2.2 2008/01/28 02:47:17 rumble Exp $	*/

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

/* md.c -- sgimips machine specific routines */

#include <sys/types.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <stdio.h>
#include <curses.h>
#include <unistd.h>
#include <fcntl.h>
#include <util.h>
#include <errno.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

struct utsname instsys;

int
md_get_info(void)
{
	struct disklabel disklabel;
	int fd;
	char dev_name[100];

	snprintf(dev_name, 100, "/dev/r%s%c", diskdev, 'a' + getrawpartition());

	fd = open(dev_name, O_RDONLY, 0);
	if (fd < 0) {
		if (logging)
			(void)fprintf(logfp, "Can't open %s\n", dev_name);
		endwin();
		fprintf(stderr, "Can't open %s\n", dev_name);
		exit(1);
	}
	if (ioctl(fd, DIOCGDINFO, &disklabel) == -1) {
		if (logging)
			(void)fprintf(logfp, "Can't read disklabel on %s.\n",
				dev_name);
		endwin();
		fprintf(stderr, "Can't read disklabel on %s.\n", dev_name);
		close(fd);
		exit(1);
	}
	close(fd);

	dlcyl = disklabel.d_ncylinders;
	dlhead = disklabel.d_ntracks;
	dlsec = disklabel.d_nsectors;
	sectorsize = disklabel.d_secsize;
	dlcylsize = disklabel.d_secpercyl;

	/*
	 * Compute whole disk size. Take max of (dlcyl*dlhead*dlsec)
	 * and secperunit,  just in case the disk is already labelled.
	 * (If our new label's RAW_PART size ends up smaller than the
	 * in-core RAW_PART size  value, updating the label will fail.)
	 */
	dlsize = dlcyl*dlhead*dlsec;
	if (disklabel.d_secperunit > dlsize)
		dlsize = disklabel.d_secperunit;

	return 1;
}

int
md_pre_disklabel(void)
{
	return 0;
}

int
md_post_disklabel(void)
{
	set_swap(diskdev, bsdlabel);
        if (strstr(instsys.version, "(INSTALL32_IP3x)"))   
		return run_program(RUN_DISPLAY,
		    "%s %s", "/usr/mdec/sgivol -f -w boot /usr/mdec/ip3xboot",
		    diskdev);

	if (strstr(instsys.version, "(INSTALL32_IP2x)")) {
		run_program(RUN_DISPLAY,
		  "%s %s", "/usr/mdec/sgivol -f -w aoutboot /usr/mdec/aoutboot",
		  diskdev);
		return run_program(RUN_DISPLAY,
		  "%s %s", "/usr/mdec/sgivol -f -w boot /usr/mdec/ip2xboot",
		  diskdev);
	}

	/* Presumably an IP12, we add the boot code later... */
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


/* update support */
int 
md_update(void)
{
	/* endwin(); */
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

	enable_rc_conf();

	run_program(0, "rm -f %s", target_expand("/sysinst"));
	run_program(0, "rm -f %s", target_expand("/.termcap"));
	run_program(0, "rm -f %s", target_expand("/.profile"));
	
	if (strstr(instsys.version, "(GENERIC32_IP12)"))
		run_program(0, "/usr/mdec/sgivol -f -w netbsd %s %s",
			    target_expand("/netbsd.ecoff"), diskdev);
}

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
md_init_set_status(int minimal)
{
	(void)minimal;

        /*
         * Get the name of the Install Kernel we are running under and
         * enable the installation of the corresponding GENERIC kernel.
         */
        uname(&instsys);
        if (strstr(instsys.version, "(INSTALL32_IP3x)"))
                set_kernel_set(SET_KERNEL_2);
        else if (strstr(instsys.version, "(INSTALL32_IP2x)"))
                set_kernel_set(SET_KERNEL_1);
	else if (strstr(instsys.version, "(GENERIC32_IP12)"))
		set_kernel_set(SET_KERNEL_3);
}

int
md_post_extract(void)
{
	return 0;
}

/*	$NetBSD: md.c,v 1.2.6.2 2014/08/20 00:05:15 tls Exp $ */

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

/* md.c -- hpcarm machine specific routines */

#include <stdio.h>
#include <util.h>
#include <sys/param.h>
#include <machine/cpu.h>
#include <sys/sysctl.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"
#include "endian.h"
#include "mbr.h"

void
md_init(void)
{
}

void
md_init_set_status(int flags)
{
	static const struct {
		const char *name;
		const int set;
	} kern_sets[] = {
		{ "IPAQ",	SET_KERNEL_IPAQ },
		{ "JORNADA720",	SET_KERNEL_JORNADA720 },
		{ "WZERO3",	SET_KERNEL_WZERO3 }
	};
	static const int mib[2] = {CTL_KERN, KERN_VERSION};
	size_t len;
	char *version;
	u_int i;

	/* check INSTALL kernel name to select an appropriate kernel set */
	/* XXX: hw.cpu_model has a processor name on arm ports */
	sysctl(mib, 2, NULL, &len, NULL, 0);
	version = malloc(len);
	if (version == NULL)
		return;
	sysctl(mib, 2, version, &len, NULL, 0);
	for (i = 0; i < __arraycount(kern_sets); i++) {
		if (strstr(version, kern_sets[i].name) != NULL) {
			set_kernel_set(kern_sets[i].set);
			break;
		}
	}
	free(version);
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
	msg_display(MSG_dofdisk);

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
		msg_display(MSG_dobad144);
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
	struct mbr_sector pbr;
	char adevname[STRSIZE];
	ssize_t sz;
	int fd = -1;

	snprintf(adevname, sizeof(adevname), "/dev/r%sa", pm->diskdev);
	fd = open(adevname, O_RDWR);
	if (fd < 0)
		goto out;

	/* Read partition boot record */
	sz = pread(fd, &pbr, sizeof(pbr), 0);
	if (sz != sizeof(pbr))
		goto out;

	/* Check magic number */
	if (pbr.mbr_magic != le16toh(MBR_MAGIC))
		goto out;

#define	OSNAME	"NetBSD60"
	/* Update oemname */
	memcpy(&pbr.mbr_oemname, OSNAME, sizeof(OSNAME) - 1);

	/* Clear BPB */
	memset(&pbr.mbr_bpb, 0, sizeof(pbr.mbr_bpb));

	/* write-backed new patition boot record */
	(void)pwrite(fd, &pbr, sizeof(pbr), 0);

out:
	if (fd >= 0)
		close(fd);
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
	enable_rc_conf();
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

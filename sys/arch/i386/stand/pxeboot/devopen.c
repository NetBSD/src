/*	$NetBSD: devopen.c,v 1.7.92.1 2009/05/13 17:17:52 jym Exp $	*/

/*
 * Copyright 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <lib/libsa/stand.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <net.h>

#include <libi386.h>
#ifdef _STANDALONE
#include <lib/libkern/libkern.h>
#include <bootinfo.h>
#else
#include <string.h>
#endif

#include "pxeboot.h"

#ifdef _STANDALONE
struct btinfo_bootpath bibp;
#endif

/*
 * Open the "device" named by the combined file system/file name
 * given as the fname arg.  Format is:
 *
 *	nfs:netbsd
 *	tftp:netbsd
 *	netbsd
 *
 * If no file system is specified, we default to the first in the
 * file system table (which ought to be NFS).
 *
 * We always open just one device (the PXE netif).
 */
int
devopen(struct open_file *f, const char *fname, char **file)
{
	struct devsw *dp;
	char *filename;
	size_t fsnamelen;
	int i, error;

	dp = &devsw[0];

	/* Set the default boot file system. */
	memcpy( file_system, pxeboot_fstab[0].fst_ops, sizeof(struct fs_ops));

	/* if we got passed a filename, pass it to the BOOTP server */
	if (fname)
		strncpy(bootfile, fname, FNAME_SIZE);

	/* Open the device; this might give us a boot file name. */
	error = (*dp->dv_open)(f, NULL);
	if (error)
		return (error);

	f->f_dev = dp;

	/*
	 * If the DHCP server provided a file name:
	 * - If it contains a ":", assume it points to a NetBSD kernel.
	 * - If not, assume that the DHCP server was not able to pass
	 *   a separate filename for the kernel. (The name probably
	 *   was the same as used to load "pxeboot".) Ignore it and
	 *   use the default in this case.
	 * So we cater to simple DHCP servers while being able to
	 * use the power of conditional behaviour in modern ones.
	 */
	if (strchr(bootfile, ':'))
		fname = bootfile;

	filename = (fname ? strchr(fname, ':') : NULL);
	if (filename != NULL) {
		fsnamelen = (size_t)((const char *)filename - fname);
		for (i = 0; i < npxeboot_fstab; i++) {
			if (strncmp(fname, pxeboot_fstab[i].fst_name,
			    fsnamelen) == 0) {
				memcpy( file_system, pxeboot_fstab[i].fst_ops,
				    sizeof(struct fs_ops));
				break;
			}
		}
		if (i == npxeboot_fstab) {
			printf("Invalid file system type specified in %s\n",
			    fname);
			error = EINVAL;
			goto bad;
		}
		filename++;
		if (filename[0] == '\0') {
			printf("No file specified in %s\n", fname);
			error = EINVAL;
			goto bad;
		}
	} else
		filename = (char *)fname;

	*file = filename;

#ifdef _STANDALONE
	strncpy(bibp.bootpath, filename, sizeof(bibp.bootpath));
	BI_ADD(&bibp, BTINFO_BOOTPATH, sizeof(bibp));
#endif

	return (0);
 bad:
	(*dp->dv_close)(f);
	f->f_dev = NULL;
	return (error);
}

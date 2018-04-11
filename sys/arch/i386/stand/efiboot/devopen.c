/*	$NetBSD: devopen.c,v 1.5 2018/04/11 10:32:09 nonaka Exp $	 */

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Bang Jun-Young.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1996, 1997
 *	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "efiboot.h"

#include <lib/libsa/dev_net.h>

#include <biosdisk.h>
#include "devopen.h"
#include <bootinfo.h>
#include "efidisk.h"

static int
dev2bios(char *devname, int unit, int *biosdev)
{

	if (strcmp(devname, "hd") == 0)
		*biosdev = 0x80 + unit;
	else if (strcmp(devname, "cd") == 0)
		*biosdev = 0x80 + get_harddrives() + unit;
	else
		return ENXIO;

	return 0;
}

void
bios2dev(int biosdev, daddr_t sector, char **devname, int *unit, int *partition)
{

	*unit = biosdev & 0x7f;

	if (efi_bootdp_type == BOOT_DEVICE_TYPE_NET) {
		*devname = "net";
		*unit = efi_net_get_booted_interface_unit();
		if (*unit < 0)
			*unit = 0;
		*partition = 0;
		return;
	} else if (biosdev >= 0x80 + get_harddrives()) {
		*devname = "cd";
		*unit -= get_harddrives();
	} else
		*devname = "hd";

	*partition = biosdisk_findpartition(biosdev, sector);
}

struct btinfo_bootpath bibp;
extern bool kernel_loaded;

/*
 * Open the EFI disk device
 */
int
devopen(struct open_file *f, const char *fname, char **file)
{
#if defined(SUPPORT_NFS) || defined(SUPPORT_TFTP)
	static const char *net_devnames[] = {
#if defined(SUPPORT_NFS)
	    "nfs",
#endif
#if defined(SUPPORT_TFTP)
	    "tftp",
#endif
	};
#endif
	struct devdesc desc;
	struct devsw *dev;
	char *fsname, *devname;
	int unit, partition;
	int biosdev;
	int i, n, error;

	error = parsebootfile(fname, &fsname, &devname, &unit, &partition,
	    (const char **) file);
	if (error)
		return error;

	memcpy(file_system, file_system_disk, sizeof(*file_system) * nfsys);
	nfsys = nfsys_disk;

#if defined(SUPPORT_NFS) || defined(SUPPORT_TFTP)
	for (i = 0; i < __arraycount(net_devnames); i++) {
		if (strcmp(devname, net_devnames[i]) == 0) {
			fsname = devname;
			devname = "net";
			break;
		}
	}
#endif

	for (i = 1; i < ndevs; i++) {
		dev = &devsw[i];
		if (strcmp(devname, DEV_NAME(dev)) == 0) {
			if (strcmp(devname, "net") == 0) {
				n = 0;
#if defined(SUPPORT_NFS)
				if (strcmp(fsname, "nfs") == 0) {
					memcpy(&file_system[n++], &file_system_nfs,
					    sizeof(file_system_nfs));
				} else
#endif
#if defined(SUPPORT_TFTP)
				if (strcmp(fsname, "tftp") == 0) {
					memcpy(&file_system[n++], &file_system_tftp,
					    sizeof(file_system_tftp));
				} else
#endif
				{
#if defined(SUPPORT_NFS)
					memcpy(&file_system[n++], &file_system_nfs,
					    sizeof(file_system_nfs));
#endif
#if defined(SUPPORT_TFTP)
					memcpy(&file_system[n++], &file_system_tftp,
					    sizeof(file_system_tftp));
#endif
				}
				nfsys = n;

				try_bootp = 1;
			}

			memset(&desc, 0, sizeof(desc));
			strlcpy(desc.d_name, devname, sizeof(desc.d_name));
			desc.d_unit = unit;

			f->f_dev = dev;
			if (!kernel_loaded) {
				strncpy(bibp.bootpath, *file,
				    sizeof(bibp.bootpath));
				BI_ADD(&bibp, BTINFO_BOOTPATH, sizeof(bibp));
			}
			return DEV_OPEN(f->f_dev)(f, &desc);
		}
	}

	/*
	 * biosdisk
	 */
	if (strcmp(devname, "esp") == 0) {
		bios2dev(boot_biosdev, boot_biossector, &devname, &unit,
		    &partition);
		if (efidisk_get_efi_system_partition(boot_biosdev, &partition))
			return ENXIO;
	}

	error = dev2bios(devname, unit, &biosdev);
	if (error)
		return error;

	f->f_dev = &devsw[0];		/* must be biosdisk */

	if (!kernel_loaded) {
		strncpy(bibp.bootpath, *file, sizeof(bibp.bootpath));
		BI_ADD(&bibp, BTINFO_BOOTPATH, sizeof(bibp));
	}

	return biosdisk_open(f, biosdev, partition);
}

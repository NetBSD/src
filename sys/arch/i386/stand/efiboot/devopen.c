/*	$NetBSD: devopen.c,v 1.13 2021/12/27 12:19:27 simonb Exp $	 */

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
#include <lib/libsa/net.h>

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
bios2dev(int biosdev, daddr_t sector, char **devname, int *unit,
	 int *partition, const char **part_name)
{
	static char savedevname[MAXDEVNAME+1];

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

	(void)biosdisk_findpartition(biosdev, sector, partition, part_name);
	if (part_name != NULL && *part_name != NULL) {
		snprintf(savedevname, sizeof(savedevname),
		    "NAME=%s", *part_name);
		*devname = savedevname;
	}
}

#if defined(SUPPORT_NFS) || defined(SUPPORT_TFTP)
const struct netboot_fstab *
netboot_fstab_find(const char *name)
{
	int i;

	if (strcmp(name, "net") == 0)
		return &netboot_fstab[0];

	for (i = 0; i < nnetboot_fstab; i++) {
		if (strcmp(name, netboot_fstab[i].name) == 0)
			return &netboot_fstab[i];
	}

	return NULL;
}

static const struct netboot_fstab *
netboot_fstab_findn(const char *name, size_t len)
{
	int i;

	if (strncmp(name, "net", len) == 0)
		return &netboot_fstab[0];

	for (i = 0; i < nnetboot_fstab; i++) {
		if (strncmp(name, netboot_fstab[i].name, len) == 0)
			return &netboot_fstab[i];
	}

	return NULL;
}
#endif

struct btinfo_bootpath bibp;
extern bool kernel_loaded;

/*
 * Open the EFI disk device
 */
int
devopen(struct open_file *f, const char *fname, char **file)
{
	char *fsname, *devname;
	const char *xname = NULL;
	int unit, partition;
	int biosdev;
	int i, error;
#if defined(SUPPORT_NFS) || defined(SUPPORT_TFTP)
	struct devdesc desc;
	const struct netboot_fstab *nf;
	char *filename;
	size_t fsnamelen;
	int n;
#endif

	error = parsebootfile(fname, &fsname, &devname, &unit, &partition,
	    (const char **) file);
	if (error)
		return error;

	memcpy(file_system, file_system_disk,
	    sizeof(struct fs_ops) * nfsys_disk);
	nfsys = nfsys_disk;

	/* Search by GPT label or raidframe name */
	if (strstr(devname, "NAME=") == devname)
		xname = devname;
	if (strstr(devname, "raid") == devname)
		xname = fname;

	if (xname != NULL) {
		f->f_dev = &devsw[0];		/* must be biosdisk */

		if (!kernel_loaded) {
			strncpy(bibp.bootpath, *file, sizeof(bibp.bootpath));
			BI_ADD(&bibp, BTINFO_BOOTPATH, sizeof(bibp));
		}

		error = biosdisk_open_name(f, xname);
		return error;
	}

	/*
	 * Network
	 */
#if defined(SUPPORT_NFS) || defined(SUPPORT_TFTP)
	nf = netboot_fstab_find(devname);
	if (nf != NULL) {
		n = 0;
		if (strcmp(devname, "net") == 0) {
			for (i = 0; i < nnetboot_fstab; i++) {
				memcpy(&file_system[n++], netboot_fstab[i].ops,
				    sizeof(struct fs_ops));
			}
		} else {
			memcpy(&file_system[n++], nf->ops,
			    sizeof(struct fs_ops));
		}
		nfsys = n;

#ifdef SUPPORT_BOOTP
		try_bootp = 1;
#endif

		/* If we got passed a filename, pass it to the BOOTP server. */
		if (fname) {
			filename = strchr(fname, ':');
			if (filename != NULL)
				filename++;
			else
				filename = (char *)fname;
			strlcpy(bootfile, filename, sizeof(bootfile));
		}

		memset(&desc, 0, sizeof(desc));
		strlcpy(desc.d_name, "net", sizeof(desc.d_name));
		desc.d_unit = unit;

		f->f_dev = &devsw[1];		/* must be net */
		if (!kernel_loaded) {
			strncpy(bibp.bootpath, *file, sizeof(bibp.bootpath));
			BI_ADD(&bibp, BTINFO_BOOTPATH, sizeof(bibp));
		}
		error = DEV_OPEN(f->f_dev)(f, &desc);
		if (error)
			return error;

		/*
		 * If the DHCP server provided a file name:
		 * - If it contains a ":", assume it points to a NetBSD kernel.
		 * - If not, assume that the DHCP server was not able to pass
		 *   a separate filename for the kernel. (The name probably was
		 *   the same as used to load "efiboot".) Ignore it and use
		 *   the default in this case.
		 * So we cater to simple DHCP servers while being able to use
		 * the power of conditional behaviour in modern ones.
		 */
		filename = strchr(bootfile, ':');
		if (filename != NULL) {
			fname = bootfile;

			fsnamelen = filename - fname;
			nf = netboot_fstab_findn(fname, fsnamelen);
			if (nf == NULL ||
			    strncmp(fname, "net", fsnamelen) == 0) {
				printf("Invalid file system type specified in "
				    "%s\n", fname);
				error = EINVAL;
				goto neterr;
			}

			memcpy(file_system, nf->ops, sizeof(struct fs_ops));
			nfsys = 1;
		}

		filename = fname ? strchr(fname, ':') : NULL;
		if (filename != NULL) {
			filename++;
			if (*filename == '\0') {
				printf("No file specified in %s\n", fname);
				error = EINVAL;
				goto neterr;
			}
		} else
			filename = (char *)fname;

		*file = filename;
		return 0;

neterr:
		DEV_CLOSE(f->f_dev)(f);
		f->f_dev = NULL;
		return error;
	}
#endif

	/*
	 * biosdisk
	 */
	if (strcmp(devname, "esp") == 0) {
		bios2dev(boot_biosdev, boot_biossector, &devname, &unit,
		    &partition, NULL);
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

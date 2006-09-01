/*	$NetBSD: devopen.c,v 1.1 2006/09/01 21:26:18 uwe Exp $	*/

/*-
 *  Copyright (c) 1993 John Brezak
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR `AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.	IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include "biosdisk.h"

#include "boot.h"
#include "bootinfo.h"

static int dev2bios(char *devname, unsigned int unit, int *biosdev);
static int devlookup(char *d);

static int
dev2bios(char *devname, unsigned int unit, int *biosdev)
{

	if (strcmp(devname, "hd") == 0) {
		if (unit == 0 || unit == 1) {
			*biosdev = 0x40 + (unit << 4);
			return (0);
		}
	}
	return (ENXIO);
}

int
bios2dev(int biosdev, char **devname, u_int *unit, u_int sector, u_int *ptnp)
{

	*devname = "hd";
	*unit = (biosdev >> 4) & 1;
	*ptnp = biosdisk_findptn(biosdev, sector);
	return (0);
}

static int
devlookup(char *d)
{
	struct devsw *dp = devsw;
	int i;

	for (i = 0; i < ndevs; i++, dp++) {
		if ((dp->dv_name != NULL) && (strcmp(dp->dv_name, d) == 0)) {
			return (i);
		}
	}

	printf("No such device - Configured devices are:\n");
	for (dp = devsw, i = 0; i < ndevs; i++, dp++) {
		if (dp->dv_name != NULL) {
			printf(" %s", dp->dv_name);
		}
	}
	printf("\n");
	return (-1);
}

int
devopen(struct open_file *f, const char *fname, char **file)
{
	static struct btinfo_bootpath bibp;
	struct devsw *dp;
	char *devname;
	unsigned int dev, ctlr, unit, partition;
	int biosdev;
	int error;

#if defined(DEBUG)
	printf("devopen: fname = %s\n", fname);
#endif

	ctlr = 0;
	if ((error = parsebootfile(fname, &devname, &unit, &partition,
	    (const char **)file)) != 0) {
		return (error);
	}

#if defined(DEBUG)
	printf("devopen: devname = %s\n", devname);
#endif
	dev = devlookup(devname);
	if (dev == -1) {
#if defined(DEBUG)
		printf("devopen: devlookup failed\n");
#endif
		return (ENXIO);
	}

	dp = &devsw[dev];
	if (dp->dv_open == NULL) {
#if defined(DEBUG)
		printf("devopen: dev->dv_open() == NULL\n");
#endif
		return (ENODEV);
	}
	f->f_dev = dp;

	strncpy(bibp.bootpath, *file, sizeof(bibp.bootpath));
	BI_ADD(&bibp, BTINFO_BOOTPATH, sizeof(bibp));

	if (dev2bios(devname, unit, &biosdev) == 0) {
#if defined(DEBUG)
		printf("devopen: bios disk\n");
#endif
		return (biosdisk_open(f, biosdev, partition));
	}

#if defined(DEBUG)
	printf("devopen: dev->dv_open()\n");
#endif
	if ((error = (*dp->dv_open)(f, ctlr, unit, partition)) == 0) {
#if defined(DEBUG)
		printf("devopen: dev->dv_open() opened\n");
#endif
		return (0);
	}

	printf("%s%d%c:%s : %s\n", dp->dv_name, unit, partition + 'a', *file,
	    strerror(error));
	return (error);
}

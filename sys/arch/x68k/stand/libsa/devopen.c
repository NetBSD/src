/*	$NetBSD: devopen.c,v 1.5.14.2 2017/12/03 11:36:49 jdolecek Exp $	*/

/*
 * Copyright (c) 2001 Minoura Makoto
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/disklabel.h>
#include <machine/bootinfo.h>
#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>
#include "libx68k.h"

extern struct devspec devspec[]; /* defined in conf.c */
int devopen_open_dir = 0;

/*
 * Parse a device spec.
 *
 * [ha@]<dev><unit><part>:<file>
 *  ha   - host adaptor ("spc0", "spc1", "mha0")
 *  dev  - device name (e.g., "sd")
 *  unit - 0-7
 *  part - a-p
 */
int
devparse(const char *fname, int *ha, int *dev, int *unit, int *part,
	char **file)
{
	char const *s;
	int i;

	s = fname;

	if (strncmp(s, "spc0@", 5) == 0) {
		*ha = (X68K_BOOT_SCSIIF_SPC << 4) | 0;
		s += 5;
	} else if (strncmp(s, "spc1@", 5) == 0) {
		*ha = (X68K_BOOT_SCSIIF_SPC << 4) | 1;
		s += 5;
	} else if (strncmp(s, "mha0@", 5) == 0) {
		*ha = (X68K_BOOT_SCSIIF_MHA << 4) | 0;
		s += 5;
	} else {
		*ha = 0;
	}

	for (i = 0; devspec[i].ds_name != 0; i++) {
		if (strncmp (devspec[i].ds_name, s,
			     strlen(devspec[i].ds_name)) == 0)
			break;
	}

	if (devspec[i].ds_name == 0)
		return ENODEV;
	s += strlen(devspec[i].ds_name);
	*dev = devspec[i].ds_dev;

	if (devspec[i].ds_net) {
		*unit = 0;
		*part = 0;
	} else {
		*unit = *s++ - '0';
		if (*unit < 0 || *unit > devspec[i].ds_maxunit)
			/* bad unit */
			return ENODEV;
		*part = *s++ - 'a';
		if (*part < 0 || *part > MAXPARTITIONS)
			/* bad partition */
			return ENODEV;
	}

	if (*s++ != ':')
		return ENODEV;

	if (*s == '/') {
		s++;
		if (devopen_open_dir && *s == 0)
			s--;
	}
	*file = __UNCONST(s);

	return 0;
}

int
devopen(struct open_file *f, const char *fname, char **file)
{
	int error;
	int ha, dev, unit, part;
	struct devsw *dp = &devsw[0];

	error = devparse(fname, &ha, &dev, &unit, &part, file);
	if (error)
		return error;

	dp = &devsw[dev];

	if (dp->dv_open == NULL)
		return ENODEV;

	f->f_dev = dp;

	if ((error = (*dp->dv_open)(f, unit, part)) == 0)
		return 0;

	return error;
}

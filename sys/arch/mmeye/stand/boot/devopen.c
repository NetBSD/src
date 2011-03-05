/*	$NetBSD: devopen.c,v 1.1.2.2 2011/03/05 15:09:54 bouyer Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Rolf Grossmann.
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

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include "boot.h"

extern const struct fs_ops file_system_nfs;

/*
 * Parse a device spec.
 *
 * Format:
 *  [device:][filename]
 */
int
devparse(const char *fname, int *dev, uint8_t *unit, uint8_t *part,
    const char **file)
{
	const char *col;

	*unit = 0;	/* default to wd0a */
	*part = 0;
	*dev  = 0;
	*file = DEFKERNELNAME;

	if (fname == NULL)
		return 0;

	if ((col = strchr(fname, ':')) != NULL) {
		int devlen;
		uint8_t i, u, p;
		struct devsw *dp;
		char devname[MAXDEVNAME];

		devlen = col - fname;
		if (devlen > MAXDEVNAME)
			return EINVAL;

#define isnum(c)	(((c) >= '0') && ((c) <= '9'))
#define isalpha(c)	(((c) >= 'a') && ((c) <= 'z'))

		/* extract device name */
		for (i = 0; isalpha(fname[i]) && (i < devlen); i++)
			devname[i] = fname[i];
		devname[i] = '\0';

		/* parse [disk][unit][part] (ex. wd0a) strings */	
		if (!isnum(fname[i]))
			return EUNIT;

		/* device number */
		for (u = 0; isnum(fname[i]) && (i < devlen); i++)
			u = u * 10 + (fname[i] - '0');

		if (!isalpha(fname[i]))
			return EPART;

		/* partition number */
		p = 0;
		if (i < devlen)
			p = fname[i++] - 'a';

		if (i != devlen)
			return ENXIO;

		/* check device name */
		for (dp = devsw, i = 0; i < ndevs; dp++, i++) {
			if (dp->dv_name && !strcmp(devname, dp->dv_name))
				break;
		}

		if (i >= ndevs)
			return ENXIO;

		*unit = u;
		*part = p;
		*dev  = i;
		fname = ++col;
	}

	if (*fname)
		*file = fname;

	return 0;
}

int
devopen(struct open_file *f, const char *fname, char **file)
{
	struct devsw *dp;
	uint8_t unit, part;
	int dev, error;

	DPRINTF(("devopen(%s)\n", fname));

	if ((error = devparse(fname, &dev, &unit, &part,
	    (const char **)file)) != 0)
		return error;

	dp = &devsw[dev];
	if ((void *)dp->dv_open == (void *)nodev)
		return ENXIO;

	f->f_dev = dp;
    
	if ((error = (*dp->dv_open)(f, unit, part)) != 0)
	printf("%s%d%c: %d = %s\n", devsw[dev].dv_name,
	    unit, 'a' + part, error, strerror(error));

	return error;
}

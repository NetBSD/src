/* $NetBSD: devopen.c,v 1.2 2003/08/09 08:01:46 igy Exp $ */

/*
 * Copyright (c) 2003 Naoto Shimazaki.
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
 * THIS SOFTWARE IS PROVIDED BY NAOTO SHIMAZAKI AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE NAOTO OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: devopen.c,v 1.2 2003/08/09 08:01:46 igy Exp $");

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include "extern.h"

int
devopen(struct open_file *f, const char *fname, char **file)
{
	int		i;
	char		devname[IFNAME_SIZE];
	const char	*basename;

	for (i = 0; i < IFNAME_SIZE; i++) {
		if (fname[i] == '\0') {
			devname[i] = '\0';
			basename = &fname[i];
			break;
		}
		if (fname[i] == ':') {
			devname[i] = '\0';
			basename = &fname[i + 1];
			break;
		}
		devname[i] = fname[i];
	}

	for (i = 0; i < ndevs; i++) {
		if (strcmp(devname, devsw[i].dv_name) == 0) {
			f->f_dev = &devsw[i];
			return DEV_OPEN(f->f_dev)(f, basename, file);
		}
	}

	printf("No such device - Configured devices are:\n");
	for (i = 0; i < ndevs; i++) {
		if (devsw[i].dv_name)
			printf(" %s", devsw[i].dv_name);
	}
	printf("\n");

	return ENODEV;
}

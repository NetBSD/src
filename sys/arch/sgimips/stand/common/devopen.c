/*	$NetBSD: devopen.c,v 1.1.2.2 2002/01/10 19:48:35 thorpej Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)devopen.c	8.1 (Berkeley) 6/10/93
 */

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

/*
 * Decode the string 'fname', open the device and return the remaining
 * file name if any.
 */
int
devopen(f, fname, file)
	struct open_file *f;
	const char *fname;
	char **file;	/* out */
{
/*	int ctlr = 0, unit = 0, part = 0; */
	int rc;
	char namebuf[128];
	char devtype[16];
	const char *cp;
	char *ncp;
#if !defined(LIBSA_SINGLE_DEVICE)
	int i;
	struct devsw *dp;
#endif

	cp = fname;
	ncp = (char *)fname;

	/*
	 * look for a string like 'scsi(0)disk(0)rdisk(0)partition(0)netbsd'
	 * or 'dksc(0,0,0)netbsd'
	 */
	if (cp[0] == 's' && cp[1] == 'c' && cp[2] == 's' && cp[3] == 'i') {
		strcpy(devtype, "scsi");
	} else if (cp[0] == 'd' && cp[1] == 'k' && cp[2] == 's' && cp[3] == 'c') {
		strcpy(devtype, "dksi");
	} else
		return (ENXIO);
	while (ncp != NULL && *ncp != 0) {
		while (*ncp && *ncp++ != ')')
			;
		if (*ncp)
			cp = ncp;
	}
	strncpy(namebuf, fname, sizeof(namebuf));
	namebuf[cp - fname] = 0;

	printf("devopen: %s type %s file %s\n", namebuf, devtype, cp);
#ifdef LIBSA_SINGLE_DEVICE
	rc = DEV_OPEN(dp)(f, fname);
#else /* !LIBSA_SINGLE_DEVICE */
	for (dp = devsw, i = 0; i < ndevs; dp++, i++)
		if (dp->dv_name && strcmp(devtype, dp->dv_name) == 0)
			goto fnd;
	printf("Unknown device '%s'\nKnown devices are:", devtype);
	for (dp = devsw, i = 0; i < ndevs; dp++, i++)
		if (dp->dv_name)
			printf(" %s", dp->dv_name);
	printf("\n");
	return (ENXIO);

fnd:
	rc = (dp->dv_open)(f, namebuf);
#endif /* !LIBSA_SINGLE_DEVICE */
	if (rc)
		return (rc);

#ifndef LIBSA_SINGLE_DEVICE
	f->f_dev = dp;
#endif
	if (file && *cp != '\0')
		*file = (char *)cp;	/* XXX */
	return (0);
}

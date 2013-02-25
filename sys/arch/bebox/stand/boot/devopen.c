/*	$NetBSD: devopen.c,v 1.10.18.1 2013/02/25 00:28:33 tls Exp $	*/

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
#include <sys/param.h>
#include <sys/reboot.h>

static int devparse(const char *, int *, int *, int *, int *, int *, char **);


/*
 * Parse a device spec.
 *   i.e.
 *     /dev/disk/floppy
 *     /dev/disk/ide/0/master/0_n
 *     /dev/disk/ide/0/slave/0_n
 *     /dev/disk/scsi/000/0_n
 *     /dev/disk/scsi/030/0_n
 */
static int
devparse(const char *fname, int *dev, int *ctlr, int *unit, int *lunit,
	 int *part, char **file)
{
	int i;
	char devdir[] = "/dev/disk/";
	char floppy[] = "floppy";
	char ide[] = "ide";
	char scsi[] = "scsi";
	char *p;

	if (strncmp(fname, devdir, strlen(devdir)) != 0)
		return EINVAL;
	p = __UNCONST(fname) + strlen(devdir);

	if (strncmp(p, floppy, strlen(floppy)) == 0) {
		p += strlen(floppy);
		for (i = 0; devsw[i].dv_name != NULL; i++)
			if (strcmp(devsw[i].dv_name, "fd") == 0) {
				*dev = i;
				*ctlr = 0;
				*unit = 1;
				*lunit = 0;
				break;
			}
	} else if (strncmp(p, ide, strlen(ide)) == 0) {
		char master[] = "master";
		char slave[] = "slave";

		p += strlen(ide);
		if (*p++ != '/' ||
		    !isdigit(*p++) ||
		    *p++ != '/')
			return EINVAL;
		*ctlr = *(p - 2) - '0';
		if (strncmp(p, master, strlen(master)) == 0) {
			*unit = 0;
			p += strlen(master);
		} else if (strncmp(p, slave, strlen(slave)) == 0) {
			*unit = 1;
			p += strlen(slave);
		} else
			return EINVAL;
		if (*p++ != '/' ||
		    !isdigit(*p++) ||
		    *p++ != '_' ||
		    !isdigit(*p++))
			return EINVAL;
		*lunit = *(p - 3) - '0';
		*part = *(p - 1) - '0';
		for (i = 0; devsw[i].dv_name != NULL; i++)
			if (strcmp(devsw[i].dv_name, "wd") == 0) {
				*dev = i;
				break;
			}
		if (devsw[i].dv_name == NULL)
			return EINVAL;
	} else if (strncmp(p, scsi, strlen(scsi)) == 0) {
		p += strlen(scsi);
		if (*p++ != '/' ||
		    !isdigit(*p++) ||
		    !isdigit(*p++) ||
		    !isdigit(*p++) ||
		    *p++ != '/' ||
		    !isdigit(*p++) ||
		    *p++ != '_' ||
		    !isdigit(*p++))
			return EINVAL;
		*ctlr = *(p - 7) - '0';
		*unit = *(p - 6) - '0';
		*lunit = *(p - 5) - '0';
		*part = *(p - 1) - '0';
		for (i = 0; devsw[i].dv_name != NULL; i++)
			if (strcmp(devsw[i].dv_name, "sd") == 0) {
				*dev = i;
				break;
			}
		if (devsw[i].dv_name == NULL)
			return EINVAL;
	}

	if (*p++ != ':')
		return EINVAL;
	*file = p;
	return 0;
}

int
devopen(struct open_file *f, const char *fname, char **file)
{
	int error;
	int dev = 0, ctlr = 0, unit = 0, lunit = 0, part = 0;
	struct devsw *dp = &devsw[0];
	extern struct devsw pseudo_devsw;

	**file = '\0';
	error = devparse(fname, &dev, &ctlr, &unit, &lunit, &part, file);
	if (error == 0) {
		dp = &devsw[dev];
		if (!dp->dv_open)
			return ENODEV;
	} else {
		if (strcmp(fname, "in") == 0)
			/* special case: kernel in memory */
			dp = &pseudo_devsw;
		else
			return error;
	}

	f->f_dev = dp;
	if ((error = (*dp->dv_open)(f, ctlr, unit, lunit, part)) == 0)
		return 0;

	printf("%s %s\n", fname, strerror(error));

	return error;
}

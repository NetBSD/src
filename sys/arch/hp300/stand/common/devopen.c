/*	$NetBSD: devopen.c,v 1.11.26.1 2018/03/15 09:12:03 pgoyette Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/param.h>
#include <sys/reboot.h>

#include <lib/libkern/libkern.h>

#include <lib/libsa/stand.h>
#include <hp300/stand/common/samachdep.h>

u_int opendev;

#define ispart(c)	((c) >= 'a' && (c) <= 'h')

static void usage(void);
static int devlookup(const char * ,int);
static int devparse(const char *, int *, int*, int*, int*, int*, char **);

void
usage(void)
{

	printf("Usage: device(adaptor, controller, drive, partition)file\n"
	    "       <device><unit><partitionletter>:file\n");
}

static int
devlookup(const char *d, int len)
{
	struct devsw *dp = devsw;
	int i;
    
	for (i = 0; i < ndevs; i++, dp++) {
		if (dp->dv_name && strncmp(dp->dv_name, d, len) == 0) {
			/*
			 * Set the filesystem and startup up according to
			 * the device being opened.
			 */
			switch (i) {
			case 0:	/* ct */
				memcpy(file_system, file_system_rawfs,
				    sizeof(struct fs_ops));
				break;

			case 2:	/* rd */
			case 4:	/* sd */
				memcpy(file_system, file_system_ufs,
				    sizeof(struct fs_ops));
				break;

			case 6:	/* le */
				memcpy(file_system, file_system_nfs,
				    sizeof(struct fs_ops));
				break;

			default:
				/* Agh!  What happened?! */
				goto bad;
			}
		return i;
		}
	}

 bad:
	printf("No such device - Configured devices are:\n");
	for (dp = devsw, i = 0; i < ndevs; i++, dp++)
		if (dp->dv_name)
			printf(" %s", dp->dv_name);
	printf("\n");
	errno = ENODEV;
	return -1;
}

/*
 * Parse a device spec in one of two forms.
 *
 * dev(adapt, ctlr, unit, part)file
 * [A-Za-z]*[0-9]*[A-Za-z]:file
 *    dev   unit  part
 */
static int
devparse(const char *fname, int *dev, int *adapt, int *ctlr, int *unit,
    int *part, char **file)
{
	int i;
	char *s, *args[4];

	/* get device name */
	for (s = (char *)fname; *s && *s != '/' && *s != ':' && *s != '('; s++)
		continue;

	/* first form */
	if (*s == '(') {
		/* lookup device and get index */
		if ((*dev = devlookup(fname, s - fname)) < 0)
			goto baddev;

		/* tokenize device ident */
		args[0] = ++s;
		for (args[0] = s, i = 1; *s && *s != ')'; s++) {
			if (*s == ',')
				args[i++] = ++s;
		}
		switch(i) {
		case 4:
			*adapt = atoi(args[0]);
			*ctlr  = atoi(args[1]);
			*unit  = atoi(args[2]);
			*part  = atoi(args[3]);
			break;
		case 3:
			*ctlr  = atoi(args[0]);
			*unit  = atoi(args[1]);
			*part  = atoi(args[2]);
			break;
		case 2:
			*unit  = atoi(args[0]);
			*part  = atoi(args[1]);
			break;
		case 1:
			*part  = atoi(args[0]);
			break;
		case 0:
			break;
		}
		*file = ++s;
	}

	/* second form */
	else if (*s == ':') {
		int temp;

		/* isolate device */
		for (s = (char *)fname; *s != ':' && !isdigit(*s); s++)
			continue;
	
		/* lookup device and get index */
		if ((*dev = devlookup(fname, s - fname)) < 0)
			goto baddev;

		/* isolate unit */
		if ((temp = atoi(s)) > 255)
			goto bad;
		*adapt = temp / 8;
		*ctlr = temp % 8;
		for (; isdigit(*s); s++)
			continue;
	
		/* translate partition */
		if (!ispart(*s))
			goto bad;
	
		*part = *s++ - 'a';
		if (*s != ':')
			goto bad;
		*file = ++s;
	}

	/* no device present */
	else
		*file = (char *)fname;
    
	/* return the remaining unparsed part as the file to boot */
	return 0;
    
 bad:
	usage();

 baddev:
	return -1;
}    


int
devopen(struct open_file *f, const char *fname, char **file)
{
	int error;
	int dev, adapt, ctlr, unit, part;
	struct devsw *dp = &devsw[0];

	dev   = B_TYPE(bootdev);
	adapt = B_ADAPTOR(bootdev);
	ctlr  = B_CONTROLLER(bootdev);
	unit  = B_UNIT(bootdev);
	part  = B_PARTITION(bootdev);

	if ((error = devparse(fname, &dev, &adapt, &ctlr, &unit, &part, file))
	    != 0)
		return error;

	/*
	 * Set up filesystem type based on what device we're opening.
	 */
	switch (dev) {
	case 0:		/* ct */
		memcpy(file_system, file_system_rawfs, sizeof(struct fs_ops));
		break;

	case 2:		/* rd */
	case 4:		/* sd */
		memcpy(file_system, file_system_ufs, sizeof(struct fs_ops));
		break; 

	case 6:		/* le */
		memcpy(file_system, file_system_nfs, sizeof(struct fs_ops));
		break;

	default:
		/* XXX what else should we do here? */
		printf("WARNING: BOGUS BOOT DEV TYPE 0x%x!\n", dev);
		return EIO;
	}

	dp = &devsw[dev];
	
	if (!dp->dv_open)
		return ENODEV;

	f->f_dev = dp;

	if ((error = (*dp->dv_open)(f, adapt, ctlr, part)) == 0) {
		if ((error =
		    (*punitsw[dev].p_punit)(adapt, ctlr, &unit)) != 0) {
			goto bad;
		}
		opendev = MAKEBOOTDEV(dev, adapt, ctlr, unit, part);
		return 0;
	}

 bad:
	printf("%s(%d,%d,%d,%d): %s\n", devsw[dev].dv_name,
	    adapt, ctlr, unit, part, strerror(error));

	return error;
}

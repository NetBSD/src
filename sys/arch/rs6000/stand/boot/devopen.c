/*	$NetBSD: devopen.c,v 1.1.62.1 2014/08/20 00:03:21 tls Exp $	*/

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

#define	ispart(c)	((c) >= 'a' && (c) <= 'h')

int devlookup(char *);
int devparse(const char *, int *, int *, int *, int *, int *, char **);

int
devlookup(char *d)
{
	struct devsw *dp = devsw;
	int i;

	for (i = 0; i < ndevs; i++, dp++)
		if (dp->dv_name && strcmp(dp->dv_name, d) == 0)
			return (i);

	printf("No such device - Configured devices are:\n");
	for (dp = devsw, i = 0; i < ndevs; i++, dp++)
		if (dp->dv_name)
			printf(" %s", dp->dv_name);
	printf("\n");
	return (-1);
}

/*
 * Parse a device spec in one of two forms.
 *   dev(ctlr, unit, part)file
 */
int
devparse(const char *fname, int *dev, int *adapt, int *ctlr, int *unit,
	int *part, char **file)
{
	int argc, flag;
	char *s, *args[3];
	extern char nametmp[];

	/* get device name and make lower case */
	strcpy(nametmp, (char *)fname);
	for (s = nametmp; *s && *s != '('; s++)
		if (isupper(*s)) *s = tolower(*s);

	if (*s == '(') {
		/* lookup device and get index */
		*s = NULL;
		if ((*dev = devlookup(nametmp)) < 0)
		    goto baddev;

		/* tokenize device ident */
		for (++s, flag = 0, argc = 0; *s && *s != ')'; s++) {
			if (*s != ',') {
				if (!flag) {
					flag++;
					args[argc++] = s;
				}
			} else {
				if (flag) {
					*s = NULL;
					flag = 0;
				}
			}
		}
		if (*s == ')')
			*s = NULL;

		switch (argc) {
		case 3:
		    *part = atoi(args[2]);
		    /* FALL THROUGH */
		case 2:
		    *unit = atoi(args[1]);
		    /* FALL THROUGH */
		case 1:
		    *ctlr = atoi(args[0]);
		    break;
		}
		*file = ++s;
	} else {
		/* no device present */
		*file = (char *)fname;
	}
	return (0);

baddev:
	return (EINVAL);
}

int
devopen(struct open_file *f, const char *fname, char **file)
{
	int error;
	int dev = 0, ctlr = 0, unit = 0, part = 0;
	int adapt = 0;
	struct devsw *dp = &devsw[0];

	if ((error =
	    devparse(fname, &dev, &adapt, &ctlr, &unit, &part, file)) != 0)
		return (error);

	dp = &devsw[dev];
	if (!dp->dv_open)
		return (ENODEV);

	f->f_dev = dp;
	if ((error = (*dp->dv_open)(f, ctlr, unit, part)) == 0)
		return (0);

	printf("%s(%d,%d,%d): %s\n", devsw[dev].dv_name,
		ctlr, unit, part, strerror(error));

	return (error);
}

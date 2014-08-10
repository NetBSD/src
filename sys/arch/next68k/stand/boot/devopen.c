/*	$NetBSD: devopen.c,v 1.6 2014/08/10 07:40:49 isaki Exp $	*/
/*
 * Copyright (c) 1994 Rolf Grossmann
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Rolf Grossmann.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

int devlookup(const char *, int);
int devparse(const char *, int *, char *, char *, char *, char **);

int
devlookup(const char *d, int len)
{
    struct devsw *dp = devsw;
    int i;
    
    for (i = 0; i < ndevs; i++, dp++)
	if (dp->dv_name && strncmp(dp->dv_name, d, len) == 0)
	    return i;

    for (i = 0; i < len; i++)
	printf("%c", d[i]);
    printf(": no such device - Configured devices are:\n");
    for (dp = devsw, i = 0; i < ndevs; i++, dp++)
	if (dp->dv_name && (void *)dp->dv_open != (void *)nodev)
	    printf(" %s", dp->dv_name);
    printf("\n");
    errno = ENXIO;
    return -1;
}

/*
 * Parse a device spec.
 *
 * Format:
 *   dev(count, lun, part)file
 */
int
devparse(const char *fname, int *dev,
	 char *count, char *lun, char *part, char **file)
{
    int i;
    const char *s, *args[3];
    
    /* get device name */
    for (s = fname; *s && *s != '/' && *s != '('; s++)
	;

    if (*s == '(') {
	/* lookup device and get index */
	if ((*dev = devlookup(fname, s - fname)) < 0)
	    goto baddev;

	/* tokenize device ident */
	args[0] = ++s;
	for (i = 1; *s && *s != ')' && i<3; s++) {
	    if (*s == ',')
		args[i++] = ++s;
	}
	if (*s != ')')
	    goto baddev;
	
	switch(i) {
	  case 3:
	      *count  = atoi(args[0]);
	      *lun  = atoi(args[1]);
	      *part  = atoi(args[2]);
	      break;
	  case 2:
	      *lun  = atoi(args[0]);
	      *part  = atoi(args[1]);
	      break;
	  case 1:
	      *part  = atoi(args[0]);
	      break;
	  case 0:
	      break;
	}
	*file = (char *)++s;	/* XXX discard const */
    }
    /* no device present */
    else
	*file = (char *)fname;	/* XXX discard const */
    
    return 0;
    
baddev:
    return ENXIO;
}

int
devopen(struct open_file *f, const char *fname, char **file)
{
    int error;
    int dev;
    char count, lun, part;
    struct devsw *dp;

    dev   = 0;	/* default device is first in table (usually scsi disk) */
    count = 0;
    lun   = 0;
    part  = 0;

    if ((error = devparse(fname, &dev, &count, &lun, &part, file)) != 0)
	return error;

    dp = &devsw[dev];
	
    if ((void *)dp->dv_open == (void *)nodev)
	return ENXIO;

    f->f_dev = dp;
    
    if ((error = (*dp->dv_open)(f, count, lun, part)) != 0)
	printf("%s(%d,%d,%d): %d = %s\n", devsw[dev].dv_name,
	       count, lun, part, error, strerror(error));

    return error;
}    

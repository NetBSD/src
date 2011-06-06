/*	$NetBSD: raw.c,v 1.1.8.2 2011/06/06 09:05:21 jruoho Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was written by Alessandro Forin and Neil Pittman
 * at Microsoft Research and contributed to The NetBSD Foundation
 * by Microsoft Corporation.
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
#include <machine/stdarg.h>
#include <machine/emipsreg.h>

#include <sys/param.h>
#include <sys/disklabel.h>
#include <sys/endian.h>

#include "common.h"
#include "raw.h"
#include "ace.h"

/* Used to parse strings of the form "a0/99/badface" all hex
 */

static inline int hexnum(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a';
    if (c >= 'A' && c <= 'F')
        return c - 'A';
    return -1;
}

static char *getxnum(char *s, uint32_t *dest)
{
    uint32_t u = 0;
    char c;
    int v = -1;

    if ((c = *s++) == '/')
        c = *s++;
    while (c && c != '/') {
        v = hexnum(c);
        if (v < 0)
            break;
        u = (u << 4) + v;
        c = *s++;
    }
    /* did we get any */
    if (v < 0)
        return NULL;
    *dest = u;
    return s-1;
}

/* rawopen("", ctlr, unit, part);
 */
int
rawopen(struct open_file *f, ...)
{
	int ctlr, unit, part;
    char *file, *cp;
	uint32_t start_sector, sector_count, load_address;
    int er;
	size_t cnt;
	va_list ap;

	va_start(ap, f);

	ctlr = va_arg(ap, int);
	unit = va_arg(ap, int);
	part = va_arg(ap, int);
    file = va_arg(ap, char *);
	va_end(ap);

    /* See if we have that controller */
    er = aceopen(f,ctlr,unit,part);
    if (er != 0)
        return er;

    /* The string in FILE must tell us three things.. */
    cp = file;
    cp = getxnum(cp,&start_sector);
    if (cp == NULL) goto Bad;
    cp = getxnum(cp,&sector_count);
    if (cp == NULL) goto Bad;
    cp = getxnum(cp,&load_address);
    if (cp == NULL) goto Bad;

    //printf("%s -> %u %u %p\n", file, start_sector, sector_count, load_address);

    /* Read them sectors */
    er = acestrategy(f->f_devdata, F_READ, start_sector, DEV_BSIZE * sector_count,
                     (void *) load_address, &cnt);
#ifndef LIBSA_NO_DEV_CLOSE
    /* regardless, close the disk */
    aceclose(f);
#endif
    /* How did it go, are we still alive */
    if (er != 0)
        return er;

    /* Ok, say it and do it then. */
    printf("Read %u sectors from sector %u at %x, jumping to it..\n", 
           sector_count, start_sector, load_address);

    call_kernel(load_address,"raw","",0,NULL);

 Bad:
#ifndef LIBSA_NO_DEV_CLOSE
    /* regardless, close it */
    aceclose(f);
#endif
    return (ENXIO);
}

#ifndef LIBSA_NO_DEV_CLOSE
int
rawclose(struct open_file *f)
{
    /* Never gets here */
	return (0);
}
#endif

int
rawstrategy(
	void *devdata,
	int rw,
	daddr_t bn,
	size_t reqcnt,
	void *addr,
	size_t *cnt)	/* out: number of bytes transfered */
{
    /* Never gets here */
    return (EIO);
}


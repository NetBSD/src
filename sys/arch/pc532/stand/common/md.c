/*	$NetBSD: md.c,v 1.2.46.1 2004/08/03 10:38:57 skrll Exp $	*/

/*
 * Copyright (c) 1994 Matthias Pfaller.
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
 *	This product includes software developed by Matthias Pfaller.
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

#include <machine/stdarg.h>

#include <sys/param.h>

#include <lib/libsa/stand.h>

#include <pc532/stand/common/samachdep.h>


#ifndef MD_START
extern u_char stext[];
#define	MD_START ((void *)((((int)stext) & 0xffffff00) - 2800 * 512))
#endif

int
mdopen(struct open_file *f, ...)
{

	f->f_devdata = MD_START;
	return(0);
}

int
mdclose(struct open_file *f)
{

	f->f_devdata = NULL;

	return (0);
}

int
mdstrategy(void *ss, int func, daddr_t dblk, u_int size, void *buf,
    u_int *rsize)
{

	memcpy(buf, (char *)ss + (dblk << DEV_BSHIFT), size);
	*rsize = size;
	return(0);
}

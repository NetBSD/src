/*	$NetBSD: timeio.c,v 1.1 2007/03/28 19:05:53 manu Exp $	*/


/*
 * Copyright (c) 2000 Joachim Kuebart.  All rights reserved.
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
 *	This product includes software developed by Joachim Kuebart.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/localedef.h>


#include <stdio.h>
#include <stdlib.h>
#include "timeio.h"


int
__loadtime(name)
	const char *name;
{
	FILE *fp;
	struct stat st;
	unsigned char **ab, *p, *pend;
	_TimeLocale *tl;




	if ((fp = fopen(name, "r")) == NULL)
		return 0;
	if (fstat(fileno(fp), &st) != 0) {
		fclose(fp);
		return 0;
	}
	if ((tl = malloc(sizeof(*tl) + (unsigned)st.st_size)) == NULL) {
		(void) fclose(fp);
		return 0;
	}
	if (fread(tl + 1, (unsigned)st.st_size, 1, fp) != 1)
		goto bad;
	(void) fclose(fp);
	/* LINTED pointer cast */
	p = (unsigned char *)&tl[1];
	pend = p + (unsigned)st.st_size;
	/* LINTED pointer cast */
	for (ab = (unsigned char **)tl;
	     ab != (unsigned char **)&tl[1];
	     ab++) {
		*ab = p;
		while (p != pend && *p != '\n')
			p++;
		if (p == pend)
			goto bad;
		*p++ = '\0';
	}
	if (_CurrentTimeLocale != &_DefaultTimeLocale)
		/* LINTED const castaway */
		free((void *)_CurrentTimeLocale);
	_CurrentTimeLocale = tl;
	return 1;
bad:
	free(tl);
	(void) fclose(fp);
	return 0;
}


/*	$NetBSD: nsdispatch.c,v 1.1.4.2 1997/05/26 15:40:36 lukem Exp $	*/

/*-
 * Copyright (c) 1997 Luke Mewburn <lukem@netbsd.org>
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
 * 	This product includes software developed by Luke Mewburn.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "nsswitch.h"

static int	  _nsmapsize = 0;
static ns_DBT	 *_nsmap = NULL;
/*
 * number of ns_DBT per chunk in _nsmap. calculated along the lines of
 *	NSELEMSPERCHUNK * sizeof(ns_DBT) < (1024 - some slop)
 * this is to help power of 2 mallocs which are really wasteful if the
 * amount just overflows a power of 2 boundary.
 */
#define NSELEMSPERCHUNK		10

int
_nscmp(const void *a, const void *b)
{
    return strncmp(((ns_DBT*)a)->name, ((ns_DBT *)b)->name, NS_MAXDBLEN);
}

void
__nsdbput(dbt)
	const ns_DBT *dbt;
{
	int	i;

	for (i = 0; i < _nsmapsize; i++) {
		if (_nscmp(dbt, &_nsmap[i]) == 0) {
			memmove((void *)&_nsmap[i], (void *)dbt,
				sizeof(ns_DBT));
			return;
		}
	}

	if ((_nsmapsize % NSELEMSPERCHUNK) == 0) {
		_nsmap = realloc(_nsmap,
			    (_nsmapsize + NSELEMSPERCHUNK) * sizeof(ns_DBT));
		if (_nsmap == NULL)
			_err(1, "nsdispatch: %m");

	}
	memmove((void *)&_nsmap[_nsmapsize++], (void *)dbt, sizeof(ns_DBT));
}


/*
 * __nsdbget --
 *	args: ns_DBT *dbt
 *	modifies: dbt
 *	looks for the element in dbt->name and finds from there
 *
 */
void
__nsdbget(dbt)
	ns_DBT *dbt;
{
#ifdef NSLINEAR		/* XXX: just to compare linear vs bsearch */
	int	i;
	for (i = 0; i < _nsmapsize; i++) {
		if (_nscmp(dbt, &_nsmap[i]) == 0) {
			memmove((void *)dbt, (void *)&_nsmap[i],
				sizeof(ns_DBT));
			return;
		}
	}
#else
	ns_DBT *e;
	e = bsearch(dbt, _nsmap, _nsmapsize, sizeof(ns_DBT), _nscmp);
	if (e != NULL)
		memmove((void *)dbt, (void *)e, sizeof(ns_DBT));
#endif
}


void
_nsdumpdbt(dbt)
	const ns_DBT *dbt;
{
	int i;
	printf("%s:\n", dbt->name);
	for (i = 0; i < dbt->size; i++) {
		int source = (dbt->map[i] & NS_SOURCEMASK);
		int status = (dbt->map[i] & NS_STATUSMASK);

		printf("  %2d: [%x] ", i, dbt->map[i]);
		if (!dbt->map[i]) {
		    printf(" (empty)\n");
		    continue;
		}
		switch(source) {
		case NS_FILES:		printf(" files"); break;
		case NS_DNS:		printf(" dns"); break;
		case NS_NIS:		printf(" nis"); break;
		case NS_NISPLUS:	printf(" nisplus"); break;
		case NS_COMPAT:		printf(" compat"); break;
		default:		printf(" BADSOURCE(%x)", source); break;
		}

		if (!(status & NS_SUCCESS))
			printf(" SUCCESS=continue");
		if (status & NS_UNAVAIL)
			printf(" UNAVAIL=return");
		if (status & NS_NOTFOUND)
			printf(" NOTFOUND=return");
		if (status & NS_TRYAGAIN)
			printf(" TRYAGAIN=return");
		printf("\n");
	}
}


void
_nsgetdbt(src, dbt)
	const char	*src;
	ns_DBT		*dbt;
{
	static time_t	 confmod;
	struct stat	 statbuf;

	extern FILE 	*_nsyyin;
	extern int	_nsyyparse();

	strncpy(dbt->name, src, NS_MAXDBLEN);
	dbt->name[NS_MAXDBLEN - 1] = '\0';
	dbt->size = 1;
	dbt->map[0] = NS_DEFAULTMAP;		/* default to 'files' */

	if (confmod) {
		if (stat(_PATH_NS_CONF, &statbuf) == -1)
			return;
		if (confmod < statbuf.st_mtime) {
			free(_nsmap);
			_nsmap = NULL;
			_nsmapsize = 0;
			confmod = 0;
		}
	}
	if (!confmod) {
		if (stat(_PATH_NS_CONF, &statbuf) == -1)
			return;
		_nsyyin = fopen(_PATH_NS_CONF, "r");
		if (_nsyyin == NULL)
			return;
		_nsyyparse();
#ifndef NSLINEAR
		qsort(_nsmap, _nsmapsize, sizeof(ns_DBT), _nscmp);
#endif
		(void)fclose(_nsyyin);
		confmod = statbuf.st_mtime;
	}
	__nsdbget(dbt);
}


int
#if __STDC__
nsdispatch(void *retval, ns_dtab disp_tab, const char *database, ...)
#else
nsdispatch(retval, disp_tab, database, va_alist)
	void		*retval;
	ns_dtab		 disp_tab;
	const char	*database;
	va_dcl
#endif
{
	va_list	ap;
	int	curdisp, result = 0;
	ns_DBT	dbt;

	_nsgetdbt(database, &dbt);

#if NSDEBUG
	_nsdumpdbt(&dbt);
	fprintf(stderr, "nsdispatch: %s\n", database);
#endif
	for (curdisp = 0; curdisp < dbt.size; curdisp++) {
		int source = (dbt.map[curdisp] & NS_SOURCEMASK);
#if NSDEBUG
		fprintf(stderr, "    %2d: source=%d", curdisp, source);
#endif
		if (source < 0 || source >= NS_MAXSOURCE)
			continue;

		result = 0;
		if (disp_tab[source].cb) {
#if __STDC__
			va_start(ap, database);
#else
			va_start(ap);
#endif
			result = disp_tab[source].cb(retval,
						disp_tab[source].cb_data, ap);
			va_end(ap);
#if NSDEBUG
			fprintf(stderr, " result=%d (%d)", result,
			    (result & (dbt.map[curdisp] & NS_STATUSMASK)));
#endif
			if (result & (dbt.map[curdisp] & NS_STATUSMASK)) {
#if NSDEBUG
				fprintf(stderr, " MATCH!\n");
#endif
				break;
			}
		}
#if NSDEBUG
		fprintf(stderr, "\n");
#endif
	}
	return (result ? result : NS_NOTFOUND);
}

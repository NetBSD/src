/*	$NetBSD: mkheaders.c,v 1.35 2003/08/07 11:25:16 agc Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratories.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	from: @(#)mkheaders.c	8.1 (Berkeley) 6/6/93
 */

#include <sys/param.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "defs.h"

static int emitcnt(struct nvlist *);
static int emitlocs(void);
static int emitopts(void);
static int emitioconfh(void);
static int emittime(void);
static int herr(const char *, const char *, FILE *);
static int locators_print(const char *, void *, void *);
static int defopts_print(const char *, void *, void *);
static char *cntname(const char *);
static int fprintcnt(FILE *, struct nvlist *);
static int fprintstr(FILE *, const char *);

/*
 * Make the various config-generated header files.
 */
int
mkheaders(void)
{
	struct files *fi;

	/*
	 * Make headers containing counts, as needed.
	 */
	TAILQ_FOREACH(fi, &allfiles, fi_next) {
		if (fi->fi_flags & FI_HIDDEN)
			continue;
		if (fi->fi_flags & (FI_NEEDSCOUNT | FI_NEEDSFLAG) &&
		    emitcnt(fi->fi_optf))
			return (1);
	}

	if (emitopts() || emitlocs() || emitioconfh() || emittime())
		return (1);

	return (0);
}


static int
fprintcnt(FILE *fp, struct nvlist *nv)
{
	return (fprintf(fp, "#define\t%s\t%d\n",
	     cntname(nv->nv_name), nv->nv_int));
}

static int
emitcnt(struct nvlist *head)
{
	char nfname[BUFSIZ], tfname[BUFSIZ];
	struct nvlist *nv;
	FILE *fp;

	(void)snprintf(nfname, sizeof(nfname), "%s.h", head->nv_name);
	(void)snprintf(tfname, sizeof(tfname), "tmp_%s", nfname);

	if ((fp = fopen(tfname, "w")) == NULL)
		return (herr("open", tfname, NULL));

	for (nv = head; nv != NULL; nv = nv->nv_next)
		if (fprintcnt(fp, nv) < 0)
			return (herr("writ", tfname, fp));

	if (fclose(fp) == EOF)
		return (herr("clos", tfname, NULL));

	return (moveifchanged(tfname, nfname));
}

/*
 * Output a string, preceded by a tab and possibly unescaping any quotes.
 * The argument will be output as is if it doesn't start with \".
 * Otherwise the first backslash in a \? sequence will be dropped.
 */
static int
fprintstr(FILE *fp, const char *str)
{
	int n;

	if (strncmp(str, "\\\"", 2))
		return fprintf(fp, "\t%s", str);

	if (fputc('\t', fp) < 0)
		return -1;
	
	for (n = 1; *str; str++, n++) {
		switch (*str) {
		case '\\':
			if (!*++str)				/* XXX */
				str--;
		default:
			if (fputc(*str, fp) < 0)
				return -1;
			break;
		}
	}
	return n;
}

/*
 * Callback function for walking the option file hash table.  We write out
 * the options defined for this file.
 */
static int
defopts_print(const char *name, void *value, void *arg)
{
	char tfname[BUFSIZ];
	struct nvlist *nv, *option;
	int isfsoption;
	FILE *fp;

	(void)snprintf(tfname, sizeof(tfname), "tmp_%s", name);
	if ((fp = fopen(tfname, "w")) == NULL)
		return (herr("open", tfname, NULL));

	for (nv = value; nv != NULL; nv = nv->nv_next) {
		isfsoption = OPT_FSOPT(nv->nv_name);

		if ((option = ht_lookup(opttab, nv->nv_name)) == NULL &&
		    (option = ht_lookup(fsopttab, nv->nv_name)) == NULL) {
			if (fprintf(fp, "/* %s `%s' not defined */\n",
			    isfsoption ? "file system" : "option",
			    nv->nv_name) < 0)
				goto bad;
		} else {
			if (fprintf(fp, "#define\t%s", option->nv_name) < 0)
				goto bad;
			if (option->nv_str != NULL && isfsoption == 0 &&
			    fprintstr(fp, option->nv_str) < 0)
				goto bad;
			if (fputc('\n', fp) < 0)
				goto bad;
		}
	}

	if (fclose(fp) == EOF)
		return (herr("clos", tfname, NULL));

	return (moveifchanged(tfname, name));

 bad:
	return (herr("writ", tfname, fp));
}

/*
 * Emit the option header files.
 */
static int
emitopts(void)
{

	return (ht_enumerate(optfiletab, defopts_print, NULL));
}

/*
 * A callback function for walking the attribute hash table.
 * Emit CPP definitions of manifest constants for the locators on the
 * "name" attribute node (passed as the "value" parameter).
 */
static int
locators_print(const char *name, void *value, void *arg)
{
	struct attr *a;
	struct nvlist *nv;
	int i;
	char *locdup, *namedup;
	char *cp;
	FILE *fp = arg;

	a = value;
	if (a->a_locs) {
		if (strchr(name, ' ') != NULL || strchr(name, '\t') != NULL)
			/*
			 * name contains a space; we can't generate
			 * usable defines, so ignore it.
			 */
			return 0;
		locdup = estrdup(name);
		for (cp = locdup; *cp; cp++)
			if (islower(*cp))
				*cp = toupper(*cp);
		if (fprintf(fp, "extern const char *%scf_locnames[];\n",
		    name) < 0)
			return 1;
		for (i = 0, nv = a->a_locs; nv; nv = nv->nv_next, i++) {
			if (strchr(nv->nv_name, ' ') != NULL ||
			    strchr(nv->nv_name, '\t') != NULL)
				/*
				 * name contains a space; we can't generate
				 * usable defines, so ignore it.
				 */
				continue;
			namedup = estrdup(nv->nv_name);
			for (cp = namedup; *cp; cp++)
				if (islower(*cp))
					*cp = toupper(*cp);
				else if (*cp == ARRCHR)
					*cp = '_';
			if (fprintf(fp, "#define %sCF_%s %d\n",
			    locdup, namedup, i) < 0)
				return 1;
			if (nv->nv_str &&
			    fprintf(fp, "#define %sCF_%s_DEFAULT %s\n",
			    locdup, namedup, nv->nv_str) < 0)
				return 1;
			free(namedup);
		}
		free(locdup);
	}
	return 0;
}

/*
 * Build the "locators.h" file with manifest constants for all potential
 * locators in the configuration.  Do this by enumerating the attribute
 * hash table and emitting all the locators for each attribute.
 */
static int
emitlocs(void)
{
	char *tfname;
	int rval;
	FILE *tfp;
	
	tfname = "tmp_locators.h";
	if ((tfp = fopen(tfname, "w")) == NULL)
		return (herr("open", tfname, NULL));

	rval = ht_enumerate(attrtab, locators_print, tfp);
	if (fclose(tfp) == EOF)
		return (herr("clos", tfname, NULL));
	if (rval)
		return (rval);
	return (moveifchanged(tfname, "locators.h"));
}

/*
 * Build the "ioconf.h" file with extern declarations for all configured
 * cfdrivers.
 */
static int
emitioconfh(void)
{
	const char *tfname;
	FILE *tfp;
	struct devbase *d;

	tfname = "tmp_ioconf.h";
	if ((tfp = fopen(tfname, "w")) == NULL)
		return (herr("open", tfname, NULL));

	TAILQ_FOREACH(d, &allbases, d_next) {
		if (!devbase_has_instances(d, WILD))
			continue;
		if (fprintf(tfp, "extern struct cfdriver %s_cd;\n",
		    d->d_name) < 0)
			return (1);
	}

	if (fclose(tfp) == EOF)
		return (herr("clos", tfname, NULL));

	return (moveifchanged(tfname, "ioconf.h"));
}

/*
 * Make a file that config_time.h can use as a source, if required.
 */
static int
emittime(void)
{
	FILE *fp;
	time_t t;
	struct tm *tm;
	char buf[128];

	t = time(NULL);
	tm = gmtime(&t);

	if ((fp = fopen("config_time.src", "w")) == NULL)
		return (herr("open", "config_time.src", NULL));

	if (strftime(buf, sizeof(buf), "%c %Z", tm) == 0)
		return (herr("strftime", "config_time.src", NULL));

	if (fprintf(fp, "/* %s */\n"
	    "#define CONFIG_TIME\t%2lld\n"
	    "#define CONFIG_YEAR\t%2d\n"
	    "#define CONFIG_MONTH\t%2d\n"
	    "#define CONFIG_DATE\t%2d\n"
	    "#define CONFIG_HOUR\t%2d\n"
	    "#define CONFIG_MINS\t%2d\n"		
	    "#define CONFIG_SECS\t%2d\n",
	    buf, (long long)t, 
	    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
	    tm->tm_hour, tm->tm_min, tm->tm_sec) < 0)
		return (herr("fprintf", "config_time.src", NULL));

	if (fclose(fp) != 0)
		return (herr("close", "config_time.src", NULL));

	/*
	 * *Don't* moveifchanged this file.  Makefile.kern.inc will
	 * handle that if it determines such a move is necessary.
	 */
	return (0);
}

/*
 * Compare two files.  If nfname doesn't exist, or is different from
 * tfname, move tfname to nfname.  Otherwise, delete tfname.
 */
int
moveifchanged(const char *tfname, const char *nfname)
{
	char tbuf[BUFSIZ], nbuf[BUFSIZ];
	FILE *tfp, *nfp;

	if ((tfp = fopen(tfname, "r")) == NULL)
		return (herr("open", tfname, NULL));

	if ((nfp = fopen(nfname, "r")) == NULL)
		goto moveit;

	while (fgets(tbuf, sizeof(tbuf), tfp) != NULL) {
		if (fgets(nbuf, sizeof(nbuf), nfp) == NULL) {
			/*
			 * Old file has fewer lines.
			 */
			goto moveit;
		}
		if (strcmp(tbuf, nbuf) != 0)
			goto moveit;
	}

	/*
	 * We've reached the end of the new file.  Check to see if new file
	 * has fewer lines than old.
	 */
	if (fgets(nbuf, sizeof(nbuf), nfp) != NULL) {
		/*
		 * New file has fewer lines.
		 */
		goto moveit;
	}

	(void) fclose(nfp);
	(void) fclose(tfp);
	if (remove(tfname) == -1)
		return(herr("remov", tfname, NULL));
	return (0);

 moveit:
	/*
	 * They're different, or the file doesn't exist.
	 */
	if (nfp)
		(void) fclose(nfp);
	if (tfp)
		(void) fclose(tfp);
	if (rename(tfname, nfname) == -1)
		return (herr("renam", tfname, NULL));
	return (0);
}

static int
herr(const char *what, const char *fname, FILE *fp)
{

	(void)fprintf(stderr, "%s: error %sing %s: %s\n",
	    getprogname(), what, fname, strerror(errno));
	if (fp)
		(void)fclose(fp);
	return (1);
}

static char *
cntname(const char *src)
{
	char *dst, c;
	static char buf[100];

	dst = buf;
	*dst++ = 'N';
	while ((c = *src++) != 0)
		*dst++ = islower(c) ? toupper(c) : c;
	*dst = 0;
	return (buf);
}

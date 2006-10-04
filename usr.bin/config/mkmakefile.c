/*	$NetBSD: mkmakefile.c,v 1.5 2006/10/04 20:34:48 dsl Exp $	*/

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
 *	from: @(#)mkmakefile.c	8.1 (Berkeley) 6/6/93
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/param.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "sem.h"

/*
 * Make the Makefile.
 */

static const char *srcpath(struct files *); 

static const char *prefix_prologue(const char *);
static const char *filetype_prologue(struct filetype *);


static void emitdefs(FILE *);
static void emitfiles(FILE *, int, int);

static void emitobjs(FILE *);
static void emitcfiles(FILE *);
static void emitsfiles(FILE *);
static void emitrules(FILE *);
static void emitload(FILE *);
static void emitincludes(FILE *);
static void emitappmkoptions(FILE *);

int
mkmakefile(void)
{
	FILE *ifp, *ofp;
	int lineno;
	void (*fn)(FILE *);
	char *ifname;
	char line[BUFSIZ], buf[200];

	/* Try a makefile for the port first.
	 */
	(void)snprintf(buf, sizeof(buf), "arch/%s/conf/Makefile.%s",
	    machine, machine);
	ifname = sourcepath(buf);
	if ((ifp = fopen(ifname, "r")) == NULL) {
		/* Try a makefile for the architecture second.
		 */
		(void)snprintf(buf, sizeof(buf), "arch/%s/conf/Makefile.%s",
		    machinearch, machinearch);
		free(ifname);
		ifname = sourcepath(buf);
		ifp = fopen(ifname, "r");
	}
	if (ifp == NULL) {
		(void)fprintf(stderr, "config: cannot read %s: %s\n",
		    ifname, strerror(errno));
		goto bad2;
	}

	if ((ofp = fopen("Makefile.tmp", "w")) == NULL) {
		(void)fprintf(stderr, "config: cannot write Makefile: %s\n",
		    strerror(errno));
		goto bad1;
	}

	emitdefs(ofp);

	lineno = 0;
	while (fgets(line, sizeof(line), ifp) != NULL) {
		lineno++;
		if (line[0] != '%') {
			fputs(line, ofp);
			continue;
		}
		if (strcmp(line, "%OBJS\n") == 0)
			fn = emitobjs;
		else if (strcmp(line, "%CFILES\n") == 0)
			fn = emitcfiles;
		else if (strcmp(line, "%SFILES\n") == 0)
			fn = emitsfiles;
		else if (strcmp(line, "%RULES\n") == 0)
			fn = emitrules;
		else if (strcmp(line, "%LOAD\n") == 0)
			fn = emitload;
		else if (strcmp(line, "%INCLUDES\n") == 0)
			fn = emitincludes;
		else if (strcmp(line, "%MAKEOPTIONSAPPEND\n") == 0)
			fn = emitappmkoptions;
		else {
			xerror(ifname, lineno,
			    "unknown %% construct ignored: %s", line);
			continue;
		}
		(*fn)(ofp);
	}

	fflush(ofp);
	if (ferror(ofp))
		goto wrerror;

	if (ferror(ifp)) {
		(void)fprintf(stderr,
		    "config: error reading %s (at line %d): %s\n",
		    ifname, lineno, strerror(errno));
		goto bad;
	}

	if (fclose(ofp)) {
		ofp = NULL;
		goto wrerror;
	}
	(void)fclose(ifp);

	if (moveifchanged("Makefile.tmp", "Makefile") != 0) {
		(void)fprintf(stderr,
		    "config: error renaming Makefile: %s\n",
		    strerror(errno));
		goto bad2;
	}
	free(ifname);
	return (0);

 wrerror:
	(void)fprintf(stderr, "config: error writing Makefile: %s\n",
	    strerror(errno));
 bad:
	if (ofp != NULL)
		(void)fclose(ofp);
 bad1:
	(void)fclose(ifp);
	/* (void)unlink("Makefile.tmp"); */
 bad2:
	free(ifname);
	return (1);
}

/*
 * Return (possibly in a static buffer) the name of the `source' for a
 * file.  If we have `options source', or if the file is marked `always
 * source', this is always the path from the `file' line; otherwise we
 * get the .o from the obj-directory.
 */
static const char *
srcpath(struct files *fi)
{
#if 1
	/* Always have source, don't support object dirs for kernel builds. */
	return (fi->fi_path);
#else
	static char buf[MAXPATHLEN];

	if (have_source || (fi->fi_flags & FI_ALWAYSSRC) != 0)
		return (fi->fi_path);
	if (objpath == NULL) {
		error("obj-directory not set");
		return (NULL);
	}
	(void)snprintf(buf, sizeof buf, "%s/%s.o", objpath, fi->fi_base);
	return (buf);
#endif
}

static const char *
filetype_prologue(struct filetype *fit)
{
	if (fit->fit_flags & FIT_NOPROLOGUE || *fit->fit_path == '/')
		return ("");
	else
		return ("$S/");
}

static const char *
prefix_prologue(const char *path)
{
	if (*path == '/')
		return ("");
	else
		return ("$S/");
}

static void
emitdefs(FILE *fp)
{
	struct nvlist *nv;
	char *sp;

	fprintf(fp, "KERNEL_BUILD=%s\n", conffile);
	fputs("IDENT=", fp);
	sp = "";
	for (nv = options; nv != NULL; nv = nv->nv_next) {

		/* skip any options output to a header file */
		if (DEFINED_OPTION(nv->nv_name))
			continue;
		fprintf(fp, "%s-D%s", sp, nv->nv_name);
		if (nv->nv_str)
		    fprintf(fp, "=\"%s\"", nv->nv_str);
		sp = " ";
	}
	putc('\n', fp);
	fprintf(fp, "PARAM=-DMAXUSERS=%d\n", maxusers);
	fprintf(fp, "MACHINE=%s\n", machine);
	if (*srcdir == '/' || *srcdir == '.') {
		fprintf(fp, "S=\t%s\n", srcdir);
	} else {
		/*
		 * libkern and libcompat "Makefile.inc"s want relative S
		 * specification to begin with '.'.
		 */
		fprintf(fp, "S=\t./%s\n", srcdir);
	}
	for (nv = mkoptions; nv != NULL; nv = nv->nv_next)
		fprintf(fp, "%s=%s\n", nv->nv_name, nv->nv_str);
}

static void
emitobjs(FILE *fp)
{
	struct files *fi;
	struct objects *oi;
	int lpos, len, sp;

	fputs("OBJS=", fp);
	sp = '\t';
	lpos = 7;
	TAILQ_FOREACH(fi, &allfiles, fi_next) {
		if ((fi->fi_flags & FI_SEL) == 0)
			continue;
		len = strlen(fi->fi_base) + 2;
		if (lpos + len > 72) {
			fputs(" \\\n", fp);
			sp = '\t';
			lpos = 7;
		}
		fprintf(fp, "%c%s.o", sp, fi->fi_base);
		lpos += len + 1;
		sp = ' ';
	}
	TAILQ_FOREACH(oi, &allobjects, oi_next) {
		if ((oi->oi_flags & OI_SEL) == 0)
			continue;
		len = strlen(oi->oi_path);
		if (*oi->oi_path != '/')
		{
			/* e.g. "$S/" */
 			if (oi->oi_prefix != NULL)
				len += strlen(prefix_prologue(oi->oi_path)) +
				       strlen(oi->oi_prefix) + 1;
			else
				len += strlen(filetype_prologue(&oi->oi_fit));
		}
		if (lpos + len > 72) {
			fputs(" \\\n", fp);
			sp = '\t';
			lpos = 7;
		}
		if (*oi->oi_path == '/') {
			fprintf(fp, "%c%s", sp, oi->oi_path);
		} else {
			if (oi->oi_prefix != NULL) {
				fprintf(fp, "%c%s%s/%s", sp,
					    prefix_prologue(oi->oi_path),
					    oi->oi_prefix, oi->oi_path);
			} else {
				fprintf(fp, "%c%s%s", sp,
				            filetype_prologue(&oi->oi_fit),
				            oi->oi_path);
			}
		}
		lpos += len + 1;
		sp = ' ';
	}
	putc('\n', fp);
}

static void
emitcfiles(FILE *fp)
{

	emitfiles(fp, 'c', 0);
}

static void
emitsfiles(FILE *fp)
{

	emitfiles(fp, 's', 'S');
}

static void
emitfiles(FILE *fp, int suffix, int upper_suffix)
{
	struct files *fi;
	int lpos, len, sp;
	const char *fpath;
 	struct config *cf;
 	char swapname[100];

	fprintf(fp, "%cFILES=", toupper(suffix));
	sp = '\t';
	lpos = 7;
	TAILQ_FOREACH(fi, &allfiles, fi_next) {
		if ((fi->fi_flags & FI_SEL) == 0)
			continue;
		fpath = srcpath(fi);
		len = strlen(fpath);
		if (fpath[len - 1] != suffix && fpath[len - 1] != upper_suffix)
			continue;
		if (*fpath != '/') {
			/* "$S/" */
 			if (fi->fi_prefix != NULL)
				len += strlen(prefix_prologue(fi->fi_prefix)) +
				       strlen(fi->fi_prefix) + 1;
			else
				len += strlen(filetype_prologue(&fi->fi_fit));
		}
		if (lpos + len > 72) {
			fputs(" \\\n", fp);
			sp = '\t';
			lpos = 7;
		}
		if (*fi->fi_path == '/') {
			fprintf(fp, "%c%s", sp, fpath);
		} else {
			if (fi->fi_prefix != NULL) {
				fprintf(fp, "%c%s%s/%s", sp,
					    prefix_prologue(fi->fi_prefix),
					    fi->fi_prefix, fpath);
			} else {
				fprintf(fp, "%c%s%s", sp,
				            filetype_prologue(&fi->fi_fit),
				            fpath);
			}
		}
		lpos += len + 1;
		sp = ' ';
	}
 	/*
 	 * The allfiles list does not include the configuration-specific
 	 * C source files.  These files should be eliminated someday, but
 	 * for now, we have to add them to ${CFILES} (and only ${CFILES}).
 	 */
 	if (suffix == 'c') {
 		TAILQ_FOREACH(cf, &allcf, cf_next) {
 			(void)snprintf(swapname, sizeof(swapname), "swap%s.c",
 			    cf->cf_name);
 			len = strlen(swapname);
 			if (lpos + len > 72) {
 				fputs(" \\\n", fp);
 				sp = '\t';
 				lpos = 7;
 			}
 			fprintf(fp, "%c%s", sp, swapname);
 			lpos += len + 1;
 			sp = ' ';
 		}
 	}
	putc('\n', fp);
}

/*
 * Emit the make-rules.
 */
static void
emitrules(FILE *fp)
{
	struct files *fi;
	const char *cp, *fpath;
	int ch;
	char buf[200];

	TAILQ_FOREACH(fi, &allfiles, fi_next) {
		if ((fi->fi_flags & FI_SEL) == 0)
			continue;
		fpath = srcpath(fi);
		if (*fpath == '/') {
			fprintf(fp, "%s.o: %s\n", fi->fi_base, fpath);
		} else {
			if (fi->fi_prefix != NULL) {
				fprintf(fp, "%s.o: %s%s/%s\n", fi->fi_base,
					    prefix_prologue(fi->fi_prefix),
					    fi->fi_prefix, fpath);
			} else {
				fprintf(fp, "%s.o: %s%s\n",
				            fi->fi_base,
				            filetype_prologue(&fi->fi_fit),
				            fpath);
			}
		}
		if ((cp = fi->fi_mkrule) == NULL) {
			cp = "NORMAL";
			ch = fpath[strlen(fpath) - 1];
			if (islower(ch))
				ch = toupper(ch);
			(void)snprintf(buf, sizeof(buf), "${%s_%c}", cp, ch);
			cp = buf;
		}
		fprintf(fp, "\t%s\n\n", cp);
	}
}

/*
 * Emit the load commands.
 *
 * This function is not to be called `spurt'.
 */
static void
emitload(FILE *fp)
{
	struct config *cf;
	const char *nm, *swname;

	fputs(".MAIN: all\nall:", fp);
	TAILQ_FOREACH(cf, &allcf, cf_next) {
		fprintf(fp, " %s", cf->cf_name);
	}
	fputs("\n\n", fp);
	TAILQ_FOREACH(cf, &allcf, cf_next) {
		nm = cf->cf_name;
		swname =
		    cf->cf_root != NULL ? cf->cf_name : "generic";
		fprintf(fp, "KERNELS+=%s\n", nm);
		fprintf(fp, "%s: ${SYSTEM_DEP} swap${.TARGET}.o vers.o", nm);
		fprintf(fp, "\n"
			    "\t${SYSTEM_LD_HEAD}\n"
			    "\t${SYSTEM_LD} swap${.TARGET}.o\n"
			    "\t${SYSTEM_LD_TAIL}\n"
			    "\n"
			    "swap%s.o: swap%s.c\n"
			    "\t${NORMAL_C}\n\n", swname, swname);
	}
}

/*
 * Emit include headers (for any prefixes encountered)
 */
static void
emitincludes(FILE *fp)
{
	struct prefix *pf;

	SLIST_FOREACH(pf, &allprefixes, pf_next) {
		fprintf(fp, "EXTRA_INCLUDES+=\t-I%s%s\n",
		    prefix_prologue(pf->pf_prefix), pf->pf_prefix);
	}
}

static int
print_condmkopts(const char *name, void *value, void *arg)
{
	struct nvlist *nv;
	FILE *fp = arg;

	if (ht_lookup(selecttab, name) == 0)
		return (0);

	for (nv = value; nv != NULL; nv = nv->nv_next)
		fprintf(fp, "%s+=%s\n", nv->nv_name, nv->nv_str);

	return (0);
}

/*
 * Emit appending makeoptions.
 */
static void
emitappmkoptions(FILE *fp)
{
	struct nvlist *nv;

	for (nv = appmkoptions; nv != NULL; nv = nv->nv_next)
		fprintf(fp, "%s+=%s\n", nv->nv_name, nv->nv_str);

	ht_enumerate(condmkopttab, print_condmkopts, fp);
}

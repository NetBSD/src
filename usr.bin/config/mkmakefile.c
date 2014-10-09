/*	$NetBSD: mkmakefile.c,v 1.24 2014/10/09 19:33:43 uebayasi Exp $	*/

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
#include <err.h>
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
static void emitallkobjs(FILE *);
static int emitallkobjscb(const char *, void *, void *);
static void emitattrkobjs(FILE *);
static int emitattrkobjscb(const char *, void *, void *);
static void emitkobjs(FILE *);
static void emitcfiles(FILE *);
static void emitsfiles(FILE *);
static void emitrules(FILE *);
static void emitload(FILE *);
static void emitincludes(FILE *);
static void emitappmkoptions(FILE *);
static void emitsubs(FILE *, const char *, const char *, int);
static int  selectopt(const char *, void *);

/* Generate Makefile to build things per-attribute *.ko (a.k.a modular build). */
int usekobjs = 0;

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
		/*
		 * Try a makefile for the architecture second.
		 */
		(void)snprintf(buf, sizeof(buf), "arch/%s/conf/Makefile.%s",
		    machinearch, machinearch);
		free(ifname);
		ifname = sourcepath(buf);
		ifp = fopen(ifname, "r");
	}
	if (ifp == NULL) {
		warn("cannot read %s", ifname);
		goto bad2;
	}

	if ((ofp = fopen("Makefile.tmp", "w")) == NULL) {
		warn("cannot write Makefile");
		goto bad1;
	}

	emitdefs(ofp);

	lineno = 0;
	while (fgets(line, sizeof(line), ifp) != NULL) {
		lineno++;
		if ((version < 20090214 && line[0] != '%') || line[0] == '#') {
			fputs(line, ofp);
			continue;
		}
		if (strcmp(line, "%OBJS\n") == 0)
			fn = usekobjs ? emitkobjs : emitobjs;
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
		else if (strncmp(line, "%VERSION ", sizeof("%VERSION ")-1) == 0) {
			int newvers;
			if (sscanf(line, "%%VERSION %d\n", &newvers) != 1) {
				cfgxerror(ifname, lineno, "syntax error for "
				    "%%VERSION");
			} else
				setversion(newvers);
			continue;
		} else {
			if (version < 20090214)
				cfgxerror(ifname, lineno,
				    "unknown %% construct ignored: %s", line);
			else
				emitsubs(ofp, line, ifname, lineno);
			continue;
		}
		(*fn)(ofp);
	}

	fflush(ofp);
	if (ferror(ofp))
		goto wrerror;

	if (ferror(ifp)) {
		warn("error reading %s (at line %d)", ifname, lineno);
		goto bad;
	}

	if (fclose(ofp)) {
		ofp = NULL;
		goto wrerror;
	}
	(void)fclose(ifp);

	if (moveifchanged("Makefile.tmp", "Makefile") != 0) {
		warn("error renaming Makefile");
		goto bad2;
	}
	free(ifname);
	return (0);

 wrerror:
	warn("error writing Makefile");
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

static void
emitsubs(FILE *fp, const char *line, const char *file, int lineno)
{
	char *nextpct;
	const char *optname;
	struct nvlist *option;

	while (*line != '\0') {
		if (*line != '%') {
			fputc(*line++, fp);
			continue;
		}

		line++;
		nextpct = strchr(line, '%');
		if (nextpct == NULL) {
			cfgxerror(file, lineno, "unbalanced %% or "
			    "unknown construct");
			return;
		}
		*nextpct = '\0';

		if (*line == '\0')
			fputc('%', fp);
		else {
			optname = intern(line);
			if (!DEFINED_OPTION(optname)) {
				cfgxerror(file, lineno, "unknown option %s",
				    optname);
				return;
			}

			if ((option = ht_lookup(opttab, optname)) == NULL)
				option = ht_lookup(fsopttab, optname);
			if (option != NULL)
				fputs(option->nv_str ? option->nv_str : "1",
				    fp);
			/*
			 * Otherwise it's not a selected option and we don't
			 * output anything.
			 */
		}

		line = nextpct + 1;
	}
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
		cfgerror("obj-directory not set");
		return (NULL);
	}
	(void)snprintf(buf, sizeof buf, "%s/%s.o", objpath, fi->fi_base);
	return (buf);
#endif
}

static const char *
filetype_prologue(struct filetype *fit)
{
	if (*fit->fit_path == '/')
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
	const char *sp;

	fprintf(fp, "KERNEL_BUILD=%s\n", conffile);
	fputs("IDENT=", fp);
	sp = "";
	for (nv = options; nv != NULL; nv = nv->nv_next) {

		/* Skip any options output to a header file */
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

	fputs("OBJS= \\\n", fp);
	TAILQ_FOREACH(fi, &allfiles, fi_next) {
		if ((fi->fi_flags & FI_SEL) == 0)
			continue;
		fprintf(fp, "\t%s.o \\\n", fi->fi_base);
	}
	TAILQ_FOREACH(oi, &allobjects, oi_next) {
		if ((oi->oi_flags & OI_SEL) == 0)
			continue;
		if (*oi->oi_path == '/') {
			fprintf(fp, "\t%s \\\n", oi->oi_path);
		} else {
			if (oi->oi_prefix != NULL) {
				fprintf(fp, "\t%s%s/%s \\\n",
				    prefix_prologue(oi->oi_path),
				    oi->oi_prefix, oi->oi_path);
			} else {
				fprintf(fp, "\t%s%s \\\n",
				    filetype_prologue(&oi->oi_fit),
				    oi->oi_path);
			}
		}
	}
	putc('\n', fp);
}

static void
emitkobjs(FILE *fp)
{
	emitallkobjs(fp);
	emitattrkobjs(fp);
}

static void
emitallkobjs(FILE *fp)
{

	fputs("OBJS=", fp);
	ht_enumerate(attrtab, emitallkobjscb, fp);
	putc('\n', fp);
}

static int
emitallkobjscb(const char *name, void *v, void *arg)
{
	struct attr *a = v;
	FILE *fp = arg;

	if (ht_lookup(selecttab, name) == NULL)
		return 0;
	if (TAILQ_EMPTY(&a->a_files))
		return 0;
	fprintf(fp, " %s.ko", name);
	return 0;
}

static void
emitattrkobjs(FILE *fp)
{
	extern struct	hashtab *attrtab;

	ht_enumerate(attrtab, emitattrkobjscb, fp);
}

static int
emitattrkobjscb(const char *name, void *v, void *arg)
{
	struct attr *a = v;
	struct files *fi;
	FILE *fp = arg;

	if (ht_lookup(selecttab, name) == NULL)
		return 0;
	if (TAILQ_EMPTY(&a->a_files))
		return 0;
	fputc('\n', fp);
	fprintf(fp, "OBJS.%s=", name);
	TAILQ_FOREACH(fi, &a->a_files, fi_anext) {
		fprintf(fp, " %s.o", fi->fi_base);
	}
	fputc('\n', fp);
	fprintf(fp, "%s.ko: ${OBJS.%s}\n", name, name);
	fprintf(fp, "\t${LINK_O}\n");
	return 0;
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
	int len;
	const char *fpath;
 	struct config *cf;
 	char swapname[100];

	fprintf(fp, "%cFILES= \\\n", toupper(suffix));
	TAILQ_FOREACH(fi, &allfiles, fi_next) {
		if ((fi->fi_flags & FI_SEL) == 0)
			continue;
		fpath = srcpath(fi);
		len = strlen(fpath);
		if (fpath[len - 1] != suffix && fpath[len - 1] != upper_suffix)
			continue;
		if (*fi->fi_path == '/') {
			fprintf(fp, "\t%s \\\n", fpath);
		} else {
			if (fi->fi_prefix != NULL) {
				fprintf(fp, "\t%s%s/%s \\\n",
				    prefix_prologue(fi->fi_prefix),
				    fi->fi_prefix, fpath);
			} else {
				fprintf(fp, "\t%s%s \\\n",
				    filetype_prologue(&fi->fi_fit),
				    fpath);
			}
		}
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
 			fprintf(fp, "\t%s \\\n", swapname);
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
		if (fi->fi_mkrule != NULL) {
			fprintf(fp, "\t%s\n\n", fi->fi_mkrule);
		} else {
			fputs("\t${NORMAL_", fp);
			cp = strrchr(fpath, '.');
			cp = cp == NULL ? fpath : cp + 1;
			while ((ch = *cp++) != '\0') {
				fputc(toupper(ch), fp);
			}
			fputs("}\n\n", fp);
		}
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

	fputs(".MAIN: all\nall:", fp);
	TAILQ_FOREACH(cf, &allcf, cf_next) {
		fprintf(fp, " %s", cf->cf_name);
		/*
		 * If we generate multiple configs inside the same build directory
		 * with a parallel build, strange things may happen, so sequentialize
		 * them.
		 */
		if (cf != TAILQ_LAST(&allcf,conftq))
			fprintf(fp, " .WAIT");
	}
	fputs("\n\n", fp);
	TAILQ_FOREACH(cf, &allcf, cf_next) {
		fprintf(fp, "KERNELS+=%s\n", cf->cf_name);
		fprintf(fp, "%s: ${SYSTEM_DEP} swap%s.o vers.o build_kernel\n",
		    cf->cf_name, cf->cf_name);
		fprintf(fp, "swap%s.o: swap%s.c\n"
		    "\t${NORMAL_C}\n\n", cf->cf_name, cf->cf_name);
	}
	fputs("\n", fp);
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

/*
 * Emit appending makeoptions.
 */
static void
emitappmkoptions(FILE *fp)
{
	struct nvlist *nv;
	struct condexpr *cond;

	for (nv = appmkoptions; nv != NULL; nv = nv->nv_next)
		fprintf(fp, "%s+=%s\n", nv->nv_name, nv->nv_str);

	for (nv = condmkoptions; nv != NULL; nv = nv->nv_next) {
		cond = nv->nv_ptr;
		if (expr_eval(cond, selectopt, NULL))
			fprintf(fp, "%s+=%s\n", nv->nv_name, nv->nv_str);
		condexpr_destroy(cond);
		nv->nv_ptr = NULL;
	}
}

static int
/*ARGSUSED*/
selectopt(const char *name, void *context)
{

	return (ht_lookup(selecttab, strtolower(name)) != NULL);
}

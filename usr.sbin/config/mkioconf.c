/*	$NetBSD: mkioconf.c,v 1.52 1999/09/24 04:48:37 enami Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	from: @(#)mkioconf.c	8.1 (Berkeley) 6/6/93
 */

#include <sys/param.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

/*
 * Make ioconf.c.
 */
static int cf_locnames_print __P((const char *, void *, void *));
static int cforder __P((const void *, const void *));
static int emitcfdata __P((FILE *));
static int emitcfdrivers __P((FILE *));
static int emitexterns __P((FILE *));
static int emithdr __P((FILE *));
static int emitloc __P((FILE *));
static int emitpseudo __P((FILE *));
static int emitpv __P((FILE *));
static int emitroots __P((FILE *));
static int emitvfslist __P((FILE *));
static int emitname2blk __P((FILE *));

#define	SEP(pos, max)	(((u_int)(pos) % (max)) == 0 ? "\n\t" : " ")

#define ARRNAME(n, l) (strchr((n), ARRCHR) && strncmp((n), (l), strlen((l))) == 0)

/*
 * NEWLINE can only be used in the emitXXX functions.
 * In most cases it can be subsumed into an fprintf.
 */
#define	NEWLINE		if (putc('\n', fp) < 0) return (1)

int
mkioconf()
{
	FILE *fp;
	int v;

	qsort(packed, npacked, sizeof *packed, cforder);
	if ((fp = fopen("ioconf.c", "w")) == NULL) {
		(void)fprintf(stderr, "config: cannot write ioconf.c: %s\n",
		    strerror(errno));
		return (1);
	}
	v = emithdr(fp);
	if (v != 0 || emitcfdrivers(fp) || emitexterns(fp) || emitloc(fp) ||
	    emitpv(fp) || emitcfdata(fp) || emitroots(fp) || emitpseudo(fp) ||
	    emitvfslist(fp) || emitname2blk(fp)) {
		if (v >= 0)
			(void)fprintf(stderr,
			    "config: error writing ioconf.c: %s\n",
			    strerror(errno));
		(void)fclose(fp);
		/* (void)unlink("ioconf.c"); */
		return (1);
	}
	(void)fclose(fp);
	return (0);
}

static int
cforder(a, b)
	const void *a, *b;
{
	int n1, n2;

	n1 = (*(struct devi **)a)->i_cfindex;
	n2 = (*(struct devi **)b)->i_cfindex;
	return (n1 - n2);
}

static int
emithdr(ofp)
	FILE *ofp;
{
	FILE *ifp;
	int n, rv;
	char ifnbuf[200], buf[BUFSIZ];
	char *ifn;

	if (fprintf(ofp, "\
/*\n\
 * MACHINE GENERATED: DO NOT EDIT\n\
 *\n\
 * ioconf.c, from \"%s\"\n\
 */\n\n", conffile) < 0)
		return (1);

	rv = 0;
	(void)snprintf(ifnbuf, sizeof(ifnbuf), "arch/%s/conf/ioconf.incl.%s",
	    machine, machine);
	ifn = sourcepath(ifnbuf);
	if ((ifp = fopen(ifn, "r")) != NULL) {
		while ((n = fread(buf, 1, sizeof(buf), ifp)) > 0)
			if (fwrite(buf, 1, n, ofp) != n) {
				rv = 1;
				break;
			}
		if (rv == 0 && ferror(ifp)) {
			(void)fprintf(stderr, "config: error reading %s: %s\n",
			    ifn, strerror(errno));
			rv = -1;
		}
		(void)fclose(ifp);
	} else {
		if (fputs("\
#include <sys/param.h>\n\
#include <sys/conf.h>\n\
#include <sys/device.h>\n\
#include <sys/mount.h>\n", ofp) < 0)
			rv = 1;
	}
	free(ifn);
	return (rv);
}

static int
emitcfdrivers(fp)
	FILE *fp;
{
	struct devbase *d;

	NEWLINE;
	for (d = allbases; d != NULL; d = d->d_next) {
		if (!devbase_has_instances(d, WILD))
			continue;
		if (fprintf(fp, "struct cfdriver %s_cd = {\n",
			    d->d_name) < 0)
			return (1);
		if (fprintf(fp, "\tNULL, \"%s\", %s\n",
			    d->d_name, d->d_classattr != NULL ?
			    d->d_classattr->a_devclass : "DV_DULL") < 0)
			return (1);
		if (fprintf(fp, "};\n\n") < 0)
			return (1);
	}
	return (0);
}

static int
emitexterns(fp)
	FILE *fp;
{
	struct deva *da;

	NEWLINE;
	for (da = alldevas; da != NULL; da = da->d_next) {
		if (!deva_has_instances(da, WILD))
			continue;
		if (fprintf(fp, "extern struct cfattach %s_ca;\n",
			    da->d_name) < 0)
			return (1);
	}
	NEWLINE;
	return (0);
}

/*
 * Emit an initialized array of character strings describing this
 * attribute's locators.
 */
static int
cf_locnames_print(name, value, arg)
	const char *name;
	void *value;
	void *arg;
{
	struct attr *a;
	struct nvlist *nv;
	FILE *fp = arg;

	a = value;
	if (a->a_locs) {
		if (fprintf(fp, "const char *%scf_locnames[] = { ", name) < 0)
			return (1);
		for (nv = a->a_locs; nv; nv = nv->nv_next)
			if (fprintf(fp, "\"%s\", ", nv->nv_name) < 0)
				return (1);
		if (fprintf(fp, "NULL};\n") < 0)
			return (1);
	}
	return 0;
}

static int
emitloc(fp)
	FILE *fp;
{
	int i;

	if (fprintf(fp, "\n/* locators */\n\
static int loc[%d] = {", locators.used) < 0)
		return (1);
	for (i = 0; i < locators.used; i++)
		if (fprintf(fp, "%s%s,", SEP(i, 8), locators.vec[i]) < 0)
			return (1);
	if (fprintf(fp,
		    "\n};\n\nconst char *nullcf_locnames[] = {NULL};\n") < 0)
		return (1);
	return ht_enumerate(attrtab, cf_locnames_print, fp);
}

/*
 * Emit global parents-vector.
 */
static int
emitpv(fp)
	FILE *fp;
{
	int i;

	if (fprintf(fp, "\n/* parent vectors */\n\
static short pv[%d] = {", parents.used) < 0)
		return (1);
	for (i = 0; i < parents.used; i++)
		if (fprintf(fp, "%s%d,", SEP(i, 16), parents.vec[i]) < 0)
			return (1);
	return (fprintf(fp, "\n};\n") < 0);
}

/*
 * Emit the cfdata array.
 */
static int
emitcfdata(fp)
	FILE *fp;
{
	struct devi **p, *i, **par;
	int unit, v;
	const char *state, *basename, *attachment;
	struct nvlist *nv;
	struct attr *a;
	char *loc;
	char locbuf[20];
	const char *lastname = "";

	if (fprintf(fp, "\n\
#define NORM FSTATE_NOTFOUND\n\
#define STAR FSTATE_STAR\n\
\n\
struct cfdata cfdata[] = {\n\
    /* attachment       driver        unit state loc   flags parents\n\
       locnames */\n") < 0)
		return (1);
	for (p = packed; (i = *p) != NULL; p++) {
		/* the description */
		if (fprintf(fp, "/*%3d: %s at ", i->i_cfindex, i->i_name) < 0)
			return (1);
		par = i->i_parents;
		for (v = 0; v < i->i_pvlen; v++)
			if (fprintf(fp, "%s%s", v == 0 ? "" : "|",
			    i->i_parents[v]->i_name) < 0)
				return (1);
		if (v == 0 && fputs("root", fp) < 0)
			return (1);
		a = i->i_atattr;
		for (nv = a->a_locs, v = 0; nv != NULL; nv = nv->nv_next, v++) {
			if (ARRNAME(nv->nv_name, lastname)) {
				if (fprintf(fp, " %s %s",
				    nv->nv_name, i->i_locs[v]) < 0)
					return (1);
			} else {
				if (fprintf(fp, " %s %s",
					    nv->nv_name, i->i_locs[v]) < 0)
					return (1);
				lastname = nv->nv_name;
			}
		}
		if (fputs(" */\n", fp) < 0)
			return (-1);

		/* then the actual defining line */
		basename = i->i_base->d_name;
		attachment = i->i_atdeva->d_name;
		if (i->i_unit == STAR) {
			unit = i->i_base->d_umax;
			state = "STAR";
		} else {
			unit = i->i_unit;
			state = "NORM";
		}
		if (i->i_locoff >= 0) {
			(void)sprintf(locbuf, "loc+%3d", i->i_locoff);
			loc = locbuf;
		} else
			loc = "loc";
		if (fprintf(fp, "\
    {&%s_ca,%s&%s_cd,%s%2d, %s, %7s, %#6x, pv+%2d,\n\
     %scf_locnames},\n",
		    attachment, strlen(attachment) < 6 ? "\t\t" : "\t",
		    basename, strlen(basename) < 3 ? "\t\t" : "\t", unit,
		    state, loc, i->i_cfflags, i->i_pvoff,
		    a->a_locs ? a->a_name : "null") < 0)
			return (1);
	}
	return (fputs("    {0}\n};\n", fp) < 0);
}

/*
 * Emit the table of potential roots.
 */
static int
emitroots(fp)
	FILE *fp;
{
	struct devi **p, *i;

	if (fputs("\nshort cfroots[] = {\n", fp) < 0)
		return (1);
	for (p = packed; (i = *p) != NULL; p++) {
		if (i->i_at != NULL)
			continue;
		if (i->i_unit != 0 &&
		    (i->i_unit != STAR || i->i_base->d_umax != 0))
			(void)fprintf(stderr,
			    "config: warning: `%s at root' is not unit 0\n",
			    i->i_name);
		if (fprintf(fp, "\t%2d /* %s */,\n",
		    i->i_cfindex, i->i_name) < 0)
			return (1);
	}
	return (fputs("\t-1\n};\n", fp) < 0);
}

/*
 * Emit pseudo-device initialization.
 */
static int
emitpseudo(fp)
	FILE *fp;
{
	struct devi *i;
	struct devbase *d;

	if (fputs("\n/* pseudo-devices */\n", fp) < 0)
		return (1);
	for (i = allpseudo; i != NULL; i = i->i_next)
		if (fprintf(fp, "extern void %sattach __P((int));\n",
		    i->i_base->d_name) < 0)
			return (1);
	if (fputs("\nstruct pdevinit pdevinit[] = {\n", fp) < 0)
		return (1);
	for (i = allpseudo; i != NULL; i = i->i_next) {
		d = i->i_base;
		if (fprintf(fp, "\t{ %sattach, %d },\n",
		    d->d_name, d->d_umax) < 0)
			return (1);
	}
	return (fputs("\t{ 0, 0 }\n};\n", fp) < 0);
}

/*
 * Emit the initial VFS list.
 */
static int
emitvfslist(fp)
	FILE *fp;
{
	struct nvlist *nv;

	if (fputs("\n/* file systems */\n", fp) < 0)
		return (1);

	/*
	 * Walk the list twice, once to emit the externs,
	 * and again to actually emit the vfs_list_initial[]
	 * array.
	 */

	for (nv = fsoptions; nv != NULL; nv = nv->nv_next) {
		if (fprintf(fp, "extern struct vfsops %s_vfsops;\n",
		    nv->nv_str) < 0)
			return (1);
	}

	if (fputs("\nstruct vfsops *vfs_list_initial[] = {\n", fp) < 0)
		return (1);

	for (nv = fsoptions; nv != NULL; nv = nv->nv_next) {
		if (fprintf(fp, "\t&%s_vfsops,\n", nv->nv_str) < 0)
			return (1);
	}

	if (fputs("\tNULL,\n};\n", fp) < 0)
		return (1);

	return (0);
}

/*
 * Emit name to major block number table.
 */
int
emitname2blk(fp)
	FILE *fp;
{
	struct devbase *dev;

	if (fputs("\n/* device name to major block number */\n", fp) < 0)
		return (1);

	if (fprintf(fp, "struct devnametobdevmaj dev_name2blk[] = {\n") < 0)
		return (1);

	for (dev = allbases; dev != NULL; dev = dev->d_next) {
		if (dev->d_major == NODEV)
			continue;

		if (fprintf(fp, "\t{ \"%s\", %d },\n",
			    dev->d_name, dev->d_major) < 0)
			return (1);
	}
	if (fprintf(fp, "\t{ NULL, 0 }\n};\n") < 0)
		return (1);

	return (0);
}

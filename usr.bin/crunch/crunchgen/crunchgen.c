/*	$NetBSD: crunchgen.c,v 1.85.2.3 2018/07/28 04:38:13 pgoyette Exp $	*/
/*
 * Copyright (c) 1994 University of Maryland
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of U.M. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  U.M. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * U.M. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL U.M.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: James da Silva, Systems Design and Analysis Group
 *			   Computer Science Department
 *			   University of Maryland at College Park
 */
/*
 * ========================================================================
 * crunchgen.c
 *
 * Generates a Makefile and main C file for a crunched executable,
 * from specs given in a .conf file.  
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: crunchgen.c,v 1.85.2.3 2018/07/28 04:38:13 pgoyette Exp $");
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <util.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/utsname.h>

#define CRUNCH_VERSION	"20180508"

#define MAXLINELEN	16384
#define MAXFIELDS 	 2048

/* internal representation of conf file: */

/* simple lists of strings suffice for most parms */

typedef struct strlst {
    struct strlst *next;
    char *str;
} strlst_t;

/* progs have structure, each field can be set with "special" or calculated */

typedef struct prog {
    struct prog *next;
    char *name, *ident;
    char *srcdir, *objdir;
    strlst_t *objs, *objpaths;
    strlst_t *links, *keepsymbols;
    int goterror;
} prog_t;


/* global state */

static strlst_t *srcdirs = NULL;
static strlst_t *libs    = NULL;
static strlst_t *vars	  = NULL;
static prog_t   *progs   = NULL;

static char line[MAXLINELEN];

static char confname[MAXPATHLEN], infilename[MAXPATHLEN];
static char outmkname[MAXPATHLEN], outcfname[MAXPATHLEN], execfname[MAXPATHLEN];
static char cachename[MAXPATHLEN], curfilename[MAXPATHLEN];
static char curdir[MAXPATHLEN];
static char topdir[MAXPATHLEN];
static char libdir[MAXPATHLEN] = "/usr/lib";
static char dbg[MAXPATHLEN] = "-Os";
static int linenum = -1;
static int goterror = 0;

static const char *pname = "crunchgen";

/* options */
static int verbose, readcache, useobjs, oneobj, pie, libcsanitizer, sanitizer;

static int reading_cache;
static char *machine;
static char *makeobjdirprefix;
static char *makebin;
static char *makeflags;

/* general library routines */

static void status(const char *str);
__dead static void out_of_memory(void);
static void add_string(strlst_t **listp, char *str);
static int is_dir(const char *pathname);
static int is_nonempty_file(const char *pathname);

/* helper routines for main() */

__dead static void usage(void);
static void parse_conf_file(void);
static void gen_outputs(void);

extern char *crunched_skel[];

int 
main(int argc, char **argv)
{
    char *p;
    int optc;

    if ((makebin = getenv("MAKE")) == NULL)
	makebin = strdup("make");

    if ((makeflags = getenv("MAKEFLAGS")) == NULL)
	makeflags = strdup("");

    if ((machine = getenv("MACHINE")) == NULL) {
	static struct utsname utsname;

	if (uname(&utsname) == -1) {
	    perror("uname");
	    exit(1);
	}
	machine = utsname.machine;
    }
    makeobjdirprefix = getenv("MAKEOBJDIRPREFIX");
    verbose = 1;
    readcache = 1;
    useobjs = 0;
    oneobj = 1;
    pie = 0;
    *outmkname = *outcfname = *execfname = '\0';
    
    if (argc > 0)
	pname = argv[0];

    while ((optc = getopt(argc, argv, "m:c:d:e:fopqsD:L:Ov:")) != -1) {
	switch(optc) {
	case 'f':	readcache = 0; break;
	case 'p':	pie = 1; break;
	case 'q':	verbose = 0; break;
	case 'O':	oneobj = 0; break;
	case 'o':       useobjs = 1, oneobj = 0; break;
	case 's':       sanitizer = 1; break;
	case 'S':       libcsanitizer = 1; break;

	case 'm':	(void)estrlcpy(outmkname, optarg, sizeof(outmkname)); break;
	case 'c':	(void)estrlcpy(outcfname, optarg, sizeof(outcfname)); break;
	case 'e':	(void)estrlcpy(execfname, optarg, sizeof(execfname)); break;
	case 'd':       (void)estrlcpy(dbg, optarg, sizeof(dbg)); break;

	case 'D':	(void)estrlcpy(topdir, optarg, sizeof(topdir)); break;
	case 'L':	(void)estrlcpy(libdir, optarg, sizeof(libdir)); break;
	case 'v':	add_string(&vars, optarg); break;

	case '?':
	default:	usage();
	}
    }

    argc -= optind;
    argv += optind;

    if (argc != 1)
	usage();

    /* 
     * generate filenames
     */

    (void)estrlcpy(infilename, argv[0], sizeof(infilename));
    getcwd(curdir, MAXPATHLEN);

    /* confname = `basename infilename .conf` */

    if ((p = strrchr(infilename, '/')) != NULL)
	(void)estrlcpy(confname, p + 1, sizeof(confname));
    else
	(void)estrlcpy(confname, infilename, sizeof(confname));
    if ((p = strrchr(confname, '.')) != NULL && !strcmp(p, ".conf"))
	*p = '\0';

    if (!*outmkname)
	(void)snprintf(outmkname, sizeof(outmkname), "%s.mk", confname);
    if (!*outcfname)
	(void)snprintf(outcfname, sizeof(outcfname), "%s.c", confname);
    if (!*execfname)
	(void)snprintf(execfname, sizeof(execfname), "%s", confname);

    (void)snprintf(cachename, sizeof(cachename), "%s.cache", confname);

    parse_conf_file();
    gen_outputs();

    exit(goterror);
}


void 
usage(void)
{
    fprintf(stderr, 
	"%s [-fOopqSs] [-c c-file-name] [-D src-root] [-d build-options]\n"
	"\t  [-e exec-file-name] [-L lib-dir] [-m makefile-name]\n"
	"\t  [-v var-spec] conf-file\n", pname);
    exit(1);
}


/*
 * ========================================================================
 * parse_conf_file subsystem
 *
 */

/* helper routines for parse_conf_file */

static void parse_one_file(char *filename);
static void parse_line(char *line, int *fc, char **fv, int nf); 
static void add_srcdirs(int argc, char **argv);
static void add_progs(int argc, char **argv);
static void add_link(int argc, char **argv);
static void add_libs(int argc, char **argv);
static void add_special(int argc, char **argv);

static prog_t *find_prog(char *str);
static void add_prog(char *progname);


static void 
parse_conf_file(void)
{
    if (!is_nonempty_file(infilename)) {
	fprintf(stderr, "%s: fatal: input file \"%s\" not found.\n",
		pname, infilename);
	exit(1);
    }
    parse_one_file(infilename);
    if (readcache && is_nonempty_file(cachename)) {
	reading_cache = 1;
	parse_one_file(cachename);
    }
}


static void 
parse_one_file(char *filename)
{
    char *fieldv[MAXFIELDS];
    int fieldc;
    void (*f)(int c, char **v);
    FILE *cf;

    (void)snprintf(line, sizeof(line), "reading %s", filename);
    status(line);
    (void)estrlcpy(curfilename, filename, sizeof(curfilename));

    if ((cf = fopen(curfilename, "r")) == NULL) {
	perror(curfilename);
	goterror = 1;
	return;
    }

    linenum = 0;
    while (fgets(line, MAXLINELEN, cf) != NULL) {
	linenum++;
	parse_line(line, &fieldc, fieldv, MAXFIELDS);
	if (fieldc < 1)
	    continue;
	if (!strcmp(fieldv[0], "srcdirs"))	f = add_srcdirs;
	else if (!strcmp(fieldv[0], "progs"))   f = add_progs;
	else if (!strcmp(fieldv[0], "ln"))	f = add_link;
	else if (!strcmp(fieldv[0], "libs"))	f = add_libs;
	else if (!strcmp(fieldv[0], "special"))	f = add_special;
	else {
	    fprintf(stderr, "%s:%d: skipping unknown command `%s'.\n",
		    curfilename, linenum, fieldv[0]);
	    goterror = 1;
	    continue;
	}
	if (fieldc < 2) {
	    fprintf(stderr, 
		    "%s:%d: %s command needs at least 1 argument, skipping.\n",
		    curfilename, linenum, fieldv[0]);
	    goterror = 1;
	    continue;
	}
	f(fieldc, fieldv);
    }

    if (ferror(cf)) {
	perror(curfilename);
	goterror = 1;
    }
    fclose(cf);
}


static void 
parse_line(char *pline, int *fc, char **fv, int nf)
{
    char *p;

    p = pline;
    *fc = 0;
    for (;;) {
	while (isspace((unsigned char)*p))
	    p++;
	if (*p == '\0' || *p == '#')
	    break;

	if (*fc < nf)
	    fv[(*fc)++] = p;
	while (*p && !isspace((unsigned char)*p) && *p != '#')
	    p++;
	if (*p == '\0' || *p == '#')
	    break;
	*p++ = '\0';
    }
    if (*p)
	*p = '\0';		/* needed for '#' case */
}


static void 
add_srcdirs(int argc, char **argv)
{
    int i;
    char tmppath[MAXPATHLEN];

    for (i = 1; i < argc; i++) {
	if (argv[i][0] == '/')
		(void)estrlcpy(tmppath, argv[i], sizeof(tmppath));
	else {
		if (topdir[0] == '\0')
		    (void)estrlcpy(tmppath, curdir, sizeof(tmppath));
		else
		    (void)estrlcpy(tmppath, topdir, sizeof(tmppath));
		(void)estrlcat(tmppath, "/", sizeof(tmppath));
		(void)estrlcat(tmppath, argv[i], sizeof(tmppath));
	}
	if (is_dir(tmppath))
	    add_string(&srcdirs, tmppath);
	else {
	    fprintf(stderr, "%s:%d: `%s' is not a directory, skipping it.\n", 
		    curfilename, linenum, tmppath);
	    goterror = 1;
	}
    }
}


static void 
add_progs(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++)
	add_prog(argv[i]);
}


static void 
add_prog(char *progname)
{
    prog_t *p1, *p2;

    /* add to end, but be smart about dups */

    for (p1 = NULL, p2 = progs; p2 != NULL; p1 = p2, p2 = p2->next)
	if (!strcmp(p2->name, progname))
	    return;

    p2 = malloc(sizeof(prog_t));
    if (p2)
	p2->name = strdup(progname);
    if (!p2 || !p2->name) 
	out_of_memory();

    p2->next = NULL;
    if (p1 == NULL)
	progs = p2;
    else
	p1->next = p2;

    p2->ident = p2->srcdir = p2->objdir = NULL;
    p2->objs = p2->objpaths = p2->links = p2->keepsymbols = NULL;
    p2->goterror = 0;
}


static void 
add_link(int argc, char **argv)
{
    int i;
    prog_t *p = find_prog(argv[1]);

    if (p == NULL) {
	fprintf(stderr, 
		"%s:%d: no prog %s previously declared, skipping link.\n",
		curfilename, linenum, argv[1]);
	goterror = 1;
	return;
    }
    for (i = 2; i < argc; i++)
	add_string(&p->links, argv[i]);
}


static void 
add_libs(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++)
	add_string(&libs, argv[i]);
}


static void 
add_special(int argc, char **argv)
{
    int i;
    prog_t *p = find_prog(argv[1]);

    if (p == NULL) {
	if (reading_cache)
	    return;
	fprintf(stderr, 
		"%s:%d: no prog %s previously declared, skipping special.\n",
		curfilename, linenum, argv[1]);
	goterror = 1;
	return;
    }

    if (!strcmp(argv[2], "ident")) {
	if (argc != 4)
	    goto argcount;
	if ((p->ident = strdup(argv[3])) == NULL)
	    out_of_memory();
	return;
    }

    if (!strcmp(argv[2], "srcdir")) {
	if (argc != 4)
	    goto argcount;
	if (argv[3][0] == '/') {
	    if ((p->srcdir = strdup(argv[3])) == NULL)
		out_of_memory();
	} else {
	    char tmppath[MAXPATHLEN];
	    if (topdir[0] == '\0')
	        (void)estrlcpy(tmppath, curdir, sizeof(tmppath));
	    else
	        (void)estrlcpy(tmppath, topdir, sizeof(tmppath));
	    (void)estrlcat(tmppath, "/", sizeof(tmppath));
	    (void)estrlcat(tmppath, argv[3], sizeof(tmppath));
	    if ((p->srcdir = strdup(tmppath)) == NULL)
		out_of_memory();
	}
	return;
    }

    if (!strcmp(argv[2], "objdir")) {
	if (argc != 4)
	    goto argcount;
	if ((p->objdir = strdup(argv[3])) == NULL)
	    out_of_memory();
	return;
    }

    if (!strcmp(argv[2], "objs")) {
	oneobj = 0;
	for (i = 3; i < argc; i++)
	    add_string(&p->objs, argv[i]);
	return;
    }

    if (!strcmp(argv[2], "objpaths")) {
	oneobj = 0;
	for (i = 3; i < argc; i++)
	    add_string(&p->objpaths, argv[i]);
	return;
    }

    if (!strcmp(argv[2], "keepsymbols")) {
	for (i = 3; i < argc; i++)
	    add_string(&p->keepsymbols, argv[i]);
	return;
    }

    fprintf(stderr, "%s:%d: bad parameter name `%s', skipping line.\n",
	    curfilename, linenum, argv[2]);
    goterror = 1;
    return;

 argcount:
    fprintf(stderr, 
	    "%s:%d: too %s arguments, expected \"special %s %s <string>\".\n",
	    curfilename, linenum, argc < 4? "few" : "many", argv[1], argv[2]);
    goterror = 1;
}


static prog_t *
find_prog(char *str)
{
    prog_t *p;

    for (p = progs; p != NULL; p = p->next)
	if (!strcmp(p->name, str))
	    return p;

    return NULL;
}


/*
 * ========================================================================
 * gen_outputs subsystem
 *
 */

/* helper subroutines */

static void remove_error_progs(void);
static void fillin_program(prog_t *p);
static void gen_specials_cache(void);
static void gen_output_makefile(void);
static void gen_output_cfile(void);

static void fillin_program_objs(prog_t *p, char *path);
static void top_makefile_rules(FILE *outmk);
static void bottom_makefile_rules(FILE *outmk);
static void prog_makefile_rules(FILE *outmk, prog_t *p);
static void output_strlst(FILE *outf, strlst_t *lst);
static char *genident(char *str);
static char *dir_search(char *progname);


static void
gen_outputs(void)
{
    prog_t *p;

    for (p = progs; p != NULL; p = p->next)
	fillin_program(p);

    remove_error_progs();
    gen_specials_cache();
    gen_output_cfile();
    gen_output_makefile();
    status("");
    fprintf(stderr, 
	    "Run \"make -f %s objs exe\" to build crunched binary.\n",
	    outmkname);
}


static void
fillin_program(prog_t *p)
{
    char path[MAXPATHLEN];
    char *srcparent;
    strlst_t *s;

    (void)snprintf(line, sizeof(line), "filling in parms for %s", p->name);
    status(line);

    if (!p->ident) 
	p->ident = genident(p->name);
    if (!p->srcdir) {
	srcparent = dir_search(p->name);
	if (srcparent) {
	    (void)snprintf(path, sizeof(path), "%s/%s", srcparent, p->name);
	    if (is_dir(path)) {
		if (path[0] == '/') {
                    if ((p->srcdir = strdup(path)) == NULL)
			out_of_memory();
		} else {
		    char tmppath[MAXPATHLEN];
		    if (topdir[0] == '\0')
			(void)estrlcpy(tmppath, curdir, sizeof(tmppath));
		    else
			(void)estrlcpy(tmppath, topdir, sizeof(tmppath));
		    (void)estrlcat(tmppath, "/", sizeof(tmppath));
		    (void)estrlcat(tmppath, path, sizeof(tmppath));
		    if ((p->srcdir = strdup(tmppath)) == NULL)
			out_of_memory();
		}
	    }
	}
    }

    if (!p->srcdir && verbose)
	fprintf(stderr, "%s: %s: warning: could not find source directory.\n",
		infilename, p->name);

    if (!p->objdir && p->srcdir && useobjs) {
	if (makeobjdirprefix) {
	    (void)snprintf(path, sizeof(path), "%s/%s", makeobjdirprefix, p->srcdir);
	    if (is_dir(path))
		p->objdir = strdup(path);
	}
	if (!p->objdir) {
	    (void)snprintf(path, sizeof(path), "%s/obj.%s", p->srcdir, machine);
	    if (is_dir(path))
		p->objdir = strdup(path);
	}
	if (!p->objdir) {
	    (void)snprintf(path, sizeof(path), "%s/obj", p->srcdir);
	    if (is_dir(path))
		p->objdir = strdup(path);
	}
	if (!p->objdir) {
	    p->objdir = p->srcdir;
        }
    }

    if (oneobj)
	return;

    if (p->srcdir)
	(void)snprintf(path, sizeof(path), "%s/Makefile", p->srcdir);
    if (!p->objs && p->srcdir && is_nonempty_file(path))
	fillin_program_objs(p, p->srcdir);

    if (!p->objpaths && p->objs) {
	char *objdir;
	if (p->objdir && useobjs)
	    objdir = p->objdir;
	else
	    objdir = p->ident;
	for (s = p->objs; s != NULL; s = s->next) {
	    (void)snprintf(line, sizeof(line), "%s/%s", objdir, s->str);
	    add_string(&p->objpaths, line);
	}
    }
	    
    if (!p->objs && verbose)
	fprintf(stderr, "%s: %s: warning: could not find any .o files.\n", 
		infilename, p->name);

    if (!p->objpaths) {
	fprintf(stderr, 
		"%s: %s: error: no objpaths specified or calculated.\n",
		infilename, p->name);
	p->goterror = goterror = 1;
    }
}

static void
fillin_program_objs(prog_t *p, char *dirpath)
{
    char *obj, *cp;
    int rc;
    int fd;
    FILE *f;
    char tempfname[MAXPATHLEN];

    /* discover the objs from the srcdir Makefile */

    (void)snprintf(tempfname, sizeof(tempfname), "/tmp/%sXXXXXX", confname);
    if ((fd = mkstemp(tempfname)) < 0) {
	perror(tempfname);
	exit(1);
    }

    if ((f = fdopen(fd, "w")) == NULL) {
	perror(tempfname);
	goterror = 1;
	return;
    }
	
    fprintf(f, ".include \"${.CURDIR}/Makefile\"\n");
    fprintf(f, ".if defined(PROG)\n");
    fprintf(f, "OBJS?= ${PROG}.o\n");
    fprintf(f, ".endif\n");
    fprintf(f, "crunchgen_objs:\n\t@echo 'OBJS= '${OBJS}\n");
    fclose(f);

    (void)snprintf(line, sizeof(line),
	"cd %s && %s -B -f %s %s CRUNCHEDPROG=1 crunchgen_objs 2>&1", dirpath,
	makebin, tempfname, makeflags);
    if ((f = popen(line, "r")) == NULL) {
	perror("submake pipe");
	goterror = 1;
	unlink(tempfname);
	return;
    }

    while (fgets(line, MAXLINELEN, f)) {
	if (strncmp(line, "OBJS= ", 6)) {
	    if (strcmp(line,
	   	"sh: warning: running as root with dot in PATH\n") == 0)
		    continue;
	    fprintf(stderr, "make error: %s", line);
	    goterror = 1;	
	    continue;
	}
	cp = line + 6;
	while (isspace((unsigned char)*cp))
	    cp++;
	while (*cp) {
	    obj = cp;
	    while (*cp && !isspace((unsigned char)*cp))
		cp++;
	    if (*cp)
		*cp++ = '\0';
	    add_string(&p->objs, obj);
	    while (isspace((unsigned char)*cp))
		cp++;
	}
    }
    if ((rc=pclose(f)) != 0) {
	fprintf(stderr, "make error: make returned %d\n", rc);
	goterror = 1;
    }
    unlink(tempfname);
}

static void
remove_error_progs(void)
{
    prog_t *p1, *p2;

    p1 = NULL; p2 = progs; 
    while (p2 != NULL) { 
	if (!p2->goterror)
	    p1 = p2, p2 = p2->next;
	else {
	    /* delete it from linked list */
	    fprintf(stderr, "%s: %s: ignoring program because of errors.\n",
		    infilename, p2->name);
	    if (p1)
		p1->next = p2->next;
	    else 
		progs = p2->next;
	    p2 = p2->next;
	}
    }
}

static void
gen_specials_cache(void)
{
    FILE *cachef;
    prog_t *p;

    (void)snprintf(line, sizeof(line), "generating %s", cachename);
    status(line);

    if ((cachef = fopen(cachename, "w")) == NULL) {
	perror(cachename);
	goterror = 1;
	return;
    }

    fprintf(cachef, "# %s - parm cache generated from %s by crunchgen %s\n\n",
	    cachename, infilename, CRUNCH_VERSION);

    for (p = progs; p != NULL; p = p->next) {
	fprintf(cachef, "\n");
	if (p->srcdir)
	    fprintf(cachef, "special %s srcdir %s\n", p->name, p->srcdir);
	if (p->objdir && useobjs)
	    fprintf(cachef, "special %s objdir %s\n", p->name, p->objdir);
	if (p->objs) {
	    fprintf(cachef, "special %s objs", p->name);
	    output_strlst(cachef, p->objs);
	}
	if (p->objpaths) {
	    fprintf(cachef, "special %s objpaths", p->name);
	    output_strlst(cachef, p->objpaths);
	}
    }
    fclose(cachef);
}


static void
gen_output_makefile(void)
{
    prog_t *p;
    FILE *outmk;

    (void)snprintf(line, sizeof(line), "generating %s", outmkname);
    status(line);

    if ((outmk = fopen(outmkname, "w")) == NULL) {
	perror(outmkname);
	goterror = 1;
	return;
    }

    fprintf(outmk, "# %s - generated from %s by crunchgen %s\n\n",
	    outmkname, infilename, CRUNCH_VERSION);

    top_makefile_rules(outmk);

    for (p = progs; p != NULL; p = p->next)
	prog_makefile_rules(outmk, p); 

    fprintf(outmk, "\n.include <bsd.prog.mk>\n");
    fprintf(outmk, "\n# ========\n");

    bottom_makefile_rules(outmk);

    fclose(outmk);
}


static void
gen_output_cfile(void)
{
    char **cp;
    FILE *outcf;
    prog_t *p;
    strlst_t *s;

    (void)snprintf(line, sizeof(line), "generating %s", outcfname);
    status(line);

    if ((outcf = fopen(outcfname, "w")) == NULL) {
	perror(outcfname);
	goterror = 1;
	return;
    }

    fprintf(outcf, 
	  "/* %s - generated from %s by crunchgen %s */\n",
	    outcfname, infilename, CRUNCH_VERSION);

    fprintf(outcf, "#define EXECNAME \"%s\"\n", execfname);
    for (cp = crunched_skel; *cp != NULL; cp++)
	fprintf(outcf, "%s\n", *cp);

    for (p = progs; p != NULL; p = p->next)
	fprintf(outcf, "extern int _crunched_%s_stub(int, char **, char **);\n",
	    p->ident);

    fprintf(outcf, "\nstatic const struct stub entry_points[] = {\n");
    for (p = progs; p != NULL; p = p->next) {
	fprintf(outcf, "\t{ \"%s\", _crunched_%s_stub },\n",
		p->name, p->ident);
	for (s = p->links; s != NULL; s = s->next)
	    fprintf(outcf, "\t{ \"%s\", _crunched_%s_stub },\n",
		    s->str, p->ident);
    }
    
    fprintf(outcf, "\t{ EXECNAME, crunched_main },\n");
    fprintf(outcf, "\t{ NULL, NULL }\n};\n");
    fclose(outcf);
}


static char *
genident(char *str)
{
    char *n,*s,*d;

    /*
     * generates a Makefile/C identifier from a program name, mapping '-' to
     * '_' and ignoring all other non-identifier characters.  This leads to
     * programs named "foo.bar" and "foobar" to map to the same identifier.
     */

    if ((n = strdup(str)) == NULL)
	return NULL;
    for (d = s = n; *s != '\0'; s++) {
	if (*s == '-')
	    *d++ = '_';
	else 
	    if (*s == '_' || isalnum((unsigned char)*s))
		*d++ = *s;
    }
    *d = '\0';
    return n;
}


static char *
dir_search(char *progname)
{
    char path[MAXPATHLEN];
    strlst_t *dir;

    for (dir=srcdirs; dir != NULL; dir=dir->next) {
	snprintf(path, sizeof(path), "%s/%s/Makefile", dir->str, progname);
	if (is_nonempty_file(path))
	    return dir->str;
    }
    return NULL;
}


static void
top_makefile_rules(FILE *outmk)
{
    prog_t *p;

    if (!pie)
	    fprintf(outmk, "NOPIE=\n");
    if (!libcsanitizer)
	    fprintf(outmk, "NOLIBCSANITIZER=\n");
    if (!sanitizer)
	    fprintf(outmk, "NOSANITIZER=\n");
    fprintf(outmk, "NOMAN=\n\n");

    fprintf(outmk, "DBG=%s\n", dbg);
    fprintf(outmk, "MAKE?=make\n");
#ifdef NEW_TOOLCHAIN
    fprintf(outmk, "OBJCOPY?=objcopy\n");
    fprintf(outmk, "NM?=nm\n");
    fprintf(outmk, "AWK?=awk\n");
#else
    fprintf(outmk, "CRUNCHIDE?=crunchide\n");
#endif

    fprintf(outmk, "CRUNCHED_OBJS=");
    for (p = progs; p != NULL; p = p->next)
	fprintf(outmk, " %s.cro", p->name);
    fprintf(outmk, "\n");
    fprintf(outmk, "DPADD+= ${CRUNCHED_OBJS}\n");
    fprintf(outmk, "LDADD+= ${CRUNCHED_OBJS} ");
    output_strlst(outmk, libs);
    fprintf(outmk, "CRUNCHEDOBJSDIRS=");
    for (p = progs; p != NULL; p = p->next)
	fprintf(outmk, " %s", p->ident);
    fprintf(outmk, "\n\n");
    
    fprintf(outmk, "SUBMAKE_TARGETS=");
    for (p = progs; p != NULL; p = p->next)
	fprintf(outmk, " %s_make", p->ident);
    fprintf(outmk, "\n\n");

    fprintf(outmk, "LDSTATIC=-static%s\n\n", pie ? " -pie" : "");
    fprintf(outmk, "PROG=%s\n\n", execfname);
    
    fprintf(outmk, "all: ${PROG}.crunched\n");
    fprintf(outmk, "${PROG}.crunched: ${SUBMAKE_TARGETS} .WAIT ${PROG}.strip\n");
    fprintf(outmk, "${PROG}.strip:\n");
    fprintf(outmk, "\t${MAKE} -f ${PROG}.mk ${PROG}\n");
    fprintf(outmk, "\t@[ -f ${PROG}.unstripped -a ! ${PROG} -nt ${PROG}.unstripped ] || { \\\n");
    fprintf(outmk, "\t\t${_MKSHMSG:Uecho} \"  strip \" ${PROG}; \\\n");
    fprintf(outmk, "\t\tcp ${PROG} ${PROG}.unstripped && \\\n");
    fprintf(outmk, "\t\t${OBJCOPY} -S -R .eh_frame -R .eh_frame_hdr -R .note -R .note.netbsd.mcmodel -R .note.netbsd.pax -R .ident -R .comment -R .copyright ${PROG} && \\\n");
    fprintf(outmk, "\t\ttouch ${PROG}.unstripped; \\\n");
    fprintf(outmk, "\t}\n");
    fprintf(outmk, "objs: $(SUBMAKE_TARGETS)\n");
    fprintf(outmk, "exe: %s\n", execfname);
    fprintf(outmk, "clean:\n\trm -rf %s *.cro *.cro.syms *.o *_stub.c ${CRUNCHEDOBJSDIRS} ${PROG}.unstripped\n",
	    execfname);
}

static void
bottom_makefile_rules(FILE *outmk)
{
}


static void
prog_makefile_rules(FILE *outmk, prog_t *p)
{
    strlst_t *lst;

    fprintf(outmk, "\n# -------- %s\n\n", p->name);

    fprintf(outmk, "%s_OBJPATHS=", p->ident);
#ifndef NEW_TOOLCHAIN
    fprintf(outmk, " %s_stub.o", p->name);
#endif
    if (p->objs)
	output_strlst(outmk, p->objpaths);
    else
	fprintf(outmk, " %s/%s.ro\n", p->ident, p->name);

    if (p->srcdir && !useobjs) {
	fprintf(outmk, "%s_SRCDIR=%s\n", p->ident, p->srcdir);
	if (p->objs) {
	    fprintf(outmk, "%s_OBJS=", p->ident);
	    output_strlst(outmk, p->objs);
	}
	fprintf(outmk, "%s_make: %s .PHONY\n", p->ident, p->ident);
	fprintf(outmk, "\t( cd %s; printf '.PATH: ${%s_SRCDIR}\\n"
	    ".CURDIR:= ${%s_SRCDIR}\\n"
	    ".include \"$${.CURDIR}/Makefile\"\\n",
	    p->ident, p->ident, p->ident);
	for (lst = vars; lst != NULL; lst = lst->next)
	    fprintf(outmk, "%s\\n", lst->str);
	fprintf(outmk, "'\\\n");
#define MAKECMD \
    "\t| ${MAKE} -f- CRUNCHEDPROG=1 DBG=${DBG:Q} LDSTATIC=${LDSTATIC:Q} "
	fprintf(outmk, MAKECMD "depend");
	fprintf(outmk, " )\n");
	fprintf(outmk, "\t( cd %s; printf '.PATH: ${%s_SRCDIR}\\n"
	    ".CURDIR:= ${%s_SRCDIR}\\n"
	    ".include \"$${.CURDIR}/Makefile\"\\n",
	    p->ident, p->ident, p->ident);
	for (lst = vars; lst != NULL; lst = lst->next)
	    fprintf(outmk, "%s\\n", lst->str);
	fprintf(outmk, "'\\\n");
	fprintf(outmk, MAKECMD "%s %s ", libcsanitizer ? "" : "NOLIBCSANITIZER=", sanitizer ? "" : "NOSANITIZER=");
	if (p->objs)
	    fprintf(outmk, "${%s_OBJS} ) \n\n", p->ident);
	else
	    fprintf(outmk, "%s.ro ) \n\n", p->name);
    } else
        fprintf(outmk, "%s_make:\n\t@echo \"** Using existing objs for %s\"\n\n", 
		p->ident, p->name);

#ifdef NEW_TOOLCHAIN
    fprintf(outmk, "%s:\n\t mkdir %s\n", p->ident, p->ident);
#endif
    fprintf(outmk, "%s.cro: %s .WAIT ${%s_OBJPATHS}\n",
	p->name, p->ident, p->ident);

#ifdef NEW_TOOLCHAIN
    if (p->objs)
	fprintf(outmk, "\t${LD} -r -o %s/%s.ro $(%s_OBJPATHS)\n", 
		p->ident, p->name, p->ident);
    /* Use one awk command.... */
    fprintf(outmk, "\t${NM} -ng %s/%s.ro | ${AWK} '/^ *U / { next };",
	    p->ident, p->name);
    fprintf(outmk, " /^[0-9a-fA-F]+ C/ { next };");
    for (lst = p->keepsymbols; lst != NULL; lst = lst->next)
	fprintf(outmk, " / %s$$/ { next };", lst->str);
    fprintf(outmk, " / main$$/ { print \"main _crunched_%s_stub\"; next };",
	    p->ident);
    /* gdb thinks these are C++ and ignores everthing after the first $$. */
    fprintf(outmk, " { print $$3 \" \" $$3 \"$$$$from$$$$%s\" }' "
	    "> %s.cro.syms\n", p->name, p->name);
    fprintf(outmk, "\t${OBJCOPY} --redefine-syms %s.cro.syms ", p->name);
    fprintf(outmk, "%s/%s.ro %s.cro\n", p->ident, p->name, p->name);
#else
    fprintf(outmk, "\t${LD} -dc -r -o %s.cro $(%s_OBJPATHS)\n", 
		p->name, p->ident);
    fprintf(outmk, "\t${CRUNCHIDE} -k _crunched_%s_stub ", p->ident);
    for (lst = p->keepsymbols; lst != NULL; lst = lst->next)
	fprintf(outmk, "-k %s ", lst->str);
    fprintf(outmk, "%s.cro\n", p->name);
    fprintf(outmk, "%s_stub.c:\n", p->name);
    fprintf(outmk, "\techo \""
	           "int _crunched_%s_stub(int argc, char **argv, char **envp)"
	           "{return main(argc,argv,envp);}\" >%s_stub.c\n",
	    p->ident, p->name);
#endif
}

static void
output_strlst(FILE *outf, strlst_t *lst)
{
    for (; lst != NULL; lst = lst->next)
	fprintf(outf, " %s", lst->str);
    fprintf(outf, "\n");
}


/*
 * ========================================================================
 * general library routines
 *
 */

static void
status(const char *str)
{
    static int lastlen = 0;
    int len, spaces;

    if (!verbose)
	return;

    len = strlen(str);
    spaces = lastlen - len;
    if (spaces < 1)
	spaces = 1;

    fprintf(stderr, " [%s]%*.*s\r", str, spaces, spaces, " ");
    fflush(stderr);
    lastlen = len;
}


static void
out_of_memory(void)
{
    fprintf(stderr, "%s: %d: out of memory, stopping.\n", infilename, linenum);
    exit(1);
}


static void
add_string(strlst_t **listp, char *str)
{
    strlst_t *p1, *p2;

    /* add to end, but be smart about dups */

    for (p1 = NULL, p2 = *listp; p2 != NULL; p1 = p2, p2 = p2->next)
	if (!strcmp(p2->str, str))
	    return;

    p2 = malloc(sizeof(strlst_t));
    if (p2)
	p2->str = strdup(str);
    if (!p2 || !p2->str)
	out_of_memory();

    p2->next = NULL;
    if (p1 == NULL)
	*listp = p2;
    else 
	p1->next = p2;
}


static int
is_dir(const char *pathname)
{
    struct stat buf;

    if (stat(pathname, &buf) == -1)
	return 0;
    return S_ISDIR(buf.st_mode);
}

static int
is_nonempty_file(const char *pathname)
{
    struct stat buf;

    if (stat(pathname, &buf) == -1)
	return 0;

    return S_ISREG(buf.st_mode) && buf.st_size > 0;
}

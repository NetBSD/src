/*	$NetBSD: rpc_main.c,v 1.44 2015/09/20 16:57:13 kamil Exp $	*/

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user or with the express written consent of
 * Sun Microsystems, Inc.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(lint)
#if 0
static char sccsid[] = "@(#)rpc_main.c 1.30 89/03/30 (C) 1987 SMI";
#else
__RCSID("$NetBSD: rpc_main.c,v 1.44 2015/09/20 16:57:13 kamil Exp $");
#endif
#endif

/*
 * rpc_main.c, Top level of the RPC protocol compiler.
 */

#define RPCGEN_VERSION	"199506"/* This program's version (year & month) */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "rpc_scan.h"
#include "rpc_parse.h"
#include "rpc_util.h"

#define EXTEND	1		/* alias for TRUE */
#define DONT_EXTEND	0	/* alias for FALSE */

struct commandline {
	int     cflag;		/* xdr C routines */
	int     hflag;		/* header file */
	int     lflag;		/* client side stubs */
	int     mflag;		/* server side stubs */
	int     nflag;		/* netid flag */
	int     sflag;		/* server stubs for the given transport */
	int     tflag;		/* dispatch Table file */
	int     Ssflag;		/* produce server sample code */
	int     Scflag;		/* produce client sample code */
	char   *infile;		/* input module name */
	char   *outfile;	/* output module name */
};


static char *cmdname;

static const char *svcclosetime = "120";
static const char *CPP;
static char CPPFLAGS[] = "-C";
static char pathbuf[MAXPATHLEN + 1];

/* these cannot be const */
static char allv0[] = "rpcgen";
static char allv1[] = "-s";
static char allv2[] = "udp";
static char allv3[] = "-s";
static char allv4[] = "tcp";
static char *allv[] = {
	allv0,
	allv1,
	allv2,
	allv3,
	allv4,
};
static int allc = sizeof(allv) / sizeof(allv[0]);

/* these cannot be const */
static char allnv0[] = "rpcgen";
static char allnv1[] = "-s";
static char allnv2[] = "netpath";
static char *allnv[] = {
	allnv0,
	allnv1,
	allnv2,
};
static int allnc = sizeof(allnv) / sizeof(allnv[0]);

#define ARGLISTLEN	20
#define FIXEDARGS         2

static const char *arglist[ARGLISTLEN];
static int argcount = FIXEDARGS;


int     nonfatalerrors;		/* errors */
int     inetdflag /* = 1 */ ;	/* Support for inetd *//* is now the default */
int     pmflag;			/* Support for port monitors */
int     logflag;		/* Use syslog instead of fprintf for errors */
int     tblflag;		/* Support for dispatch table file */
int	BSDflag;		/* use BSD cplusplus macros */
int     callerflag;		/* Generate svc_caller() function */
int	docleanup = 1;		/* cause atexit to remove files */

#define INLINE 3
/*length at which to start doing an inline */

int     doinline = INLINE;	/* length at which to start doing an inline. 3
				 * = default if 0, no xdr_inline code */

int     indefinitewait;		/* If started by port monitors, hang till it
				 * wants */
int     exitnow;		/* If started by port monitors, exit after the
				 * call */
int     timerflag;		/* TRUE if !indefinite && !exitnow */
int     newstyle;		/* newstyle of passing arguments (by value) */
int	Mflag = 0;		/* multithread safe */
static int allfiles;		/* generate all files */
int     tirpcflag = 1;		/* generating code for tirpc, by default */

#ifdef __MSDOS__
static char *dos_cppfile = NULL;
#endif

static char *extendfile(const char *, const char *);
static void open_output(const char *, const char *);
static void add_warning(void);
static void clear_args(void);
static void open_input(const char *, const char *);
static int check_nettype(const char *, const char *[]);
static void c_output(const char *, const char *, int, const char *);
static void c_initialize(void);
static char *generate_guard(const char *);
static void h_output(const char *, const char *, int, const char *);
static void s_output(int, char *[], char *, const char *, int, const char *,
    int, int);
static void l_output(const char *, const char *, int, const char *);
static void t_output(const char *, const char *, int, const char *);
static void svc_output(const char *, const char *, int, const char *);
static void clnt_output(const char *, const char *, int, const char *);
static int do_registers(int, char *[]);
static void addarg(const char *);
static void putarg(int, const char *);
static void checkfiles(const char *, const char *);
static int parseargs(int, char *[], struct commandline *);
static void usage(void) __dead;
static void options_usage(void) __dead;

int
main(int argc, char *argv[])
{
	struct commandline cmd;

	setprogname(argv[0]);
	if ((CPP = getenv("RPCGEN_CPP")) == NULL) {
		CPP = "/usr/bin/cpp";
		if (access(CPP, X_OK))
			CPP = "/usr/bin/clang-cpp";
	}

	(void) memset((char *) &cmd, 0, sizeof(struct commandline));
	clear_args();
	atexit(crash);
	if (!parseargs(argc, argv, &cmd))
		usage();

	if (cmd.cflag || cmd.hflag || cmd.lflag || cmd.tflag || cmd.sflag ||
	    cmd.mflag || cmd.nflag || cmd.Ssflag || cmd.Scflag) {
		checkfiles(cmd.infile, cmd.outfile);
	} else
		checkfiles(cmd.infile, NULL);

	if (cmd.cflag) {
		c_output(cmd.infile, "-DRPC_XDR", DONT_EXTEND, cmd.outfile);
	} else
		if (cmd.hflag) {
			h_output(cmd.infile, "-DRPC_HDR", DONT_EXTEND, cmd.outfile);
		} else
			if (cmd.lflag) {
				l_output(cmd.infile, "-DRPC_CLNT", DONT_EXTEND, cmd.outfile);
			} else
				if (cmd.sflag || cmd.mflag || (cmd.nflag)) {
					s_output(argc, argv, cmd.infile, "-DRPC_SVC", DONT_EXTEND,
					    cmd.outfile, cmd.mflag, cmd.nflag);
				} else
					if (cmd.tflag) {
						t_output(cmd.infile, "-DRPC_TBL", DONT_EXTEND, cmd.outfile);
					} else
						if (cmd.Ssflag) {
							svc_output(cmd.infile, "-DRPC_SERVER", DONT_EXTEND, cmd.outfile);
						} else
							if (cmd.Scflag) {
								clnt_output(cmd.infile, "-DRPC_CLIENT", DONT_EXTEND, cmd.outfile);
							} else {
								/* the rescans
								 * are
								 * required,
								 * since cpp
								 * may effect
								 * input */
								c_output(cmd.infile, "-DRPC_XDR", EXTEND, "_xdr.c");
								reinitialize();
								h_output(cmd.infile, "-DRPC_HDR", EXTEND, ".h");
								reinitialize();
								l_output(cmd.infile, "-DRPC_CLNT", EXTEND, "_clnt.c");
								reinitialize();
								if (inetdflag || !tirpcflag)
									s_output(allc, allv, cmd.infile, "-DRPC_SVC", EXTEND,
									    "_svc.c", cmd.mflag, cmd.nflag);
								else
									s_output(allnc, allnv, cmd.infile, "-DRPC_SVC",
									    EXTEND, "_svc.c", cmd.mflag, cmd.nflag);
								if (tblflag) {
									reinitialize();
									t_output(cmd.infile, "-DRPC_TBL", EXTEND, "_tbl.i");
								}
								if (allfiles) {
									reinitialize();
									svc_output(cmd.infile, "-DRPC_SERVER", EXTEND, "_server.c");
								}
								if (allfiles) {
									reinitialize();
									clnt_output(cmd.infile, "-DRPC_CLIENT", EXTEND, "_client.c");
								}
							}
#ifdef __MSDOS__
	if (dos_cppfile != NULL) {
		(void) fclose(fin);
		(void) unlink(dos_cppfile);
	}
#endif
	docleanup = 0;
	exit(nonfatalerrors);
	/* NOTREACHED */
}
/*
 * add extension to filename
 */
static char *
extendfile(const char *path, const char *ext)
{
	const char *file;
	char   *res;
	const char *p;

	if ((file = strrchr(path, '/')) == NULL)
		file = path;
	else
		file++;

	res = alloc(strlen(file) + strlen(ext) + 1);
	if (res == NULL) {
		err(EXIT_FAILURE, "Out of memory");
	}
	p = strrchr(file, '.');
	if (p == NULL) {
		p = file + strlen(file);
	}
	(void) strcpy(res, file);
	(void) strcpy(res + (p - file), ext);
	return (res);
}
/*
 * Open output file with given extension
 */
static void
open_output(const char *infile, const char *outfile)
{

	if (outfile == NULL) {
		fout = stdout;
		return;
	}
	if (infile != NULL && streq(outfile, infile)) {
		errx(EXIT_FAILURE, "Output would overwrite `%s'", infile);
	}
	fout = fopen(outfile, "w");
	if (fout == NULL) {
		err(EXIT_FAILURE, "Can't open `%s'", outfile);
	}
	record_open(outfile);

}

static void
add_warning(void)
{
	f_print(fout, "/*\n");
	f_print(fout, " * Please do not edit this file.\n");
	f_print(fout, " * It was generated using rpcgen.\n");
	f_print(fout, " */\n\n");
}

/* clear list of arguments */
static void 
clear_args(void)
{
	int     i;
	for (i = FIXEDARGS; i < ARGLISTLEN; i++)
		arglist[i] = NULL;
	argcount = FIXEDARGS;
}

/*
 * Open input file with given define for C-preprocessor
 */
static void
open_input(const char *infile, const char *define)
{
	int     pd[2];

	infilename = (infile == NULL) ? "<stdin>" : infile;
#ifdef __MSDOS__
#define	DOSCPP	"\\prog\\bc31\\bin\\cpp.exe"
	{
		int     retval;
		char    drive[MAXDRIVE], dir[MAXDIR], name[MAXFILE], ext[MAXEXT];
		char    cppfile[MAXPATH];
		char   *cpp;

		if ((cpp = getenv("RPCGEN_CPP")) == NULL &&
		    (cpp = searchpath("cpp.exe")) == NULL)
			cpp = DOSCPP;

		putarg(0, cpp);
		putarg(1, "-P-");
		putarg(2, CPPFLAGS);
		addarg(define);
		addarg(infile);
		addarg(NULL);

		retval = spawnvp(P_WAIT, arglist[0], arglist);
		if (retval != 0) {
			err(EXIT_FAILURE, "C preprocessor failed");
		}
		fnsplit(infile, drive, dir, name, ext);
		fnmerge(cppfile, drive, dir, name, ".i");

		fin = fopen(cppfile, "r");
		if (fin == NULL) {
			err(EXIT_FAILURE, "Can't open `%s'", cppfile);
		}
		dos_cppfile = strdup(cppfile);
		if (dos_cppfile == NULL) {
			err(EXIT_FAILURE, "Can't copy `%s'", cppfile);
		}
	}
#else
	(void) pipe(pd);
	switch (fork()) {
	case 0:
		putarg(0, CPP);
		putarg(1, CPPFLAGS);
		addarg(define);
		addarg(infile);
		addarg(NULL);
		(void) close(1);
		(void) dup2(pd[1], 1);
		(void) close(pd[0]);
		execvp(arglist[0], __UNCONST(arglist));
		err(EXIT_FAILURE, "$RPCGEN_CPP: %s", CPP);
	case -1:
		err(EXIT_FAILURE, "fork");
	}
	(void) close(pd[1]);
	fin = fdopen(pd[0], "r");
#endif
	if (fin == NULL) {
		err(EXIT_FAILURE, "Can't open `%s'", infilename);
	}
}
/* valid tirpc nettypes */
static const char *valid_ti_nettypes[] =
{
	"netpath",
	"visible",
	"circuit_v",
	"datagram_v",
	"circuit_n",
	"datagram_n",
	"udp",
	"tcp",
	"raw",
	NULL
};
/* valid inetd nettypes */
static const char *valid_i_nettypes[] =
{
	"udp",
	"tcp",
	NULL
};

static int 
check_nettype(const char *name, const char *list_to_check[])
{
	int     i;
	for (i = 0; list_to_check[i] != NULL; i++) {
		if (strcmp(name, list_to_check[i]) == 0) {
			return 1;
		}
	}
	f_print(stderr, "illegal nettype :\'%s\'\n", name);
	return 0;
}
/*
 * Compile into an XDR routine output file
 */

static void
c_output(const char *infile, const char *define, int extend,
	 const char *outfile)
{
	definition *def;
	char   *include;
	const char *outfilename;
	long    tell;

	c_initialize();
	open_input(infile, define);
	outfilename = extend ? extendfile(infile, outfile) : outfile;
	open_output(infile, outfilename);
	add_warning();
	if (infile && (include = extendfile(infile, ".h"))) {
		f_print(fout, "#include \"%s\"\n", include);
		free(include);
		/* .h file already contains rpc/rpc.h */
	} else
		f_print(fout, "#include <rpc/rpc.h>\n");
	tell = ftell(fout);
	while ((def = get_definition()) != NULL) {
		emit(def);
	}
	if (extend && tell == ftell(fout)) {
		(void) unlink(outfilename);
	}
}


static void
c_initialize(void)
{

	/* add all the starting basic types */

	add_type(1, "int");
	add_type(1, "long");
	add_type(1, "short");
	add_type(1, "bool");

	add_type(1, "u_int");
	add_type(1, "u_long");
	add_type(1, "u_short");

}

const char    rpcgen_table_dcl[] = "struct rpcgen_table {\n\
	char	*(*proc)();\n\
	xdrproc_t	xdr_arg;\n\
	unsigned	len_arg;\n\
	xdrproc_t	xdr_res;\n\
	unsigned	len_res;\n\
};\n";


static char *
generate_guard(const char *pathname)
{
	const char *filename;
	char *guard, *tmp, *tmp2, *extdot;

	filename = strrchr(pathname, '/');	/* find last component */
	filename = ((filename == 0) ? pathname : filename + 1);
	guard = strdup(filename);
	if (guard == NULL) {
		err(EXIT_FAILURE, "strdup");
	}
	extdot = strrchr(guard, '.');

	/*
	 * Convert to valid C symbol name and make it upper case.
	 * Map non alphanumerical characters to '_'.
	 *
	 * Leave extension as it is. It will be handled in extendfile().
	 */
	for (tmp = guard; *tmp; tmp++) {
		if (islower((unsigned char)*tmp))
			*tmp = toupper((unsigned char)*tmp);
		else if (isupper((unsigned char)*tmp))
			continue;
		else if (isdigit((unsigned char)*tmp))
			continue;
		else if (*tmp == '_')
			continue;
		else if (tmp == extdot)
			break;
		else
			*tmp = '_';
	}

	/*
	 * Can't have a '_' or '.' at the front of a symbol name, beacuse it
	 * will end up as "__".
	 *
	 * Prefix it with "RPCGEN_".
	 */
	if (guard[0] == '_' || guard[0] == '.') {
		if (asprintf(&tmp2, "RPCGEN_%s", guard) == -1) {
			err(EXIT_FAILURE, "asprintf");
		}
		free(guard);
		guard = tmp2;
	}

	/* Replace the file extension */
	tmp2 = extendfile(guard, "_H_RPCGEN");
	free(guard);
	guard = tmp2;

	return (guard);
}

/*
 * Compile into an XDR header file
 */
static void
h_output(const char *infile, const char *define, int extend,
	 const char *outfile)
{
	definition *def;
	const char *outfilename;
	long    tell;
	char   *guard;
	list   *l;
	int did;

	open_input(infile, define);
	outfilename = extend ? extendfile(infile, outfile) : outfile;
	open_output(infile, outfilename);
	add_warning();
	if (outfilename || infile)
		guard = generate_guard(outfilename ? outfilename : infile);
	else {
		guard = strdup("STDIN_");
		if (guard == NULL) {
			err(EXIT_FAILURE, "strdup");
		}
	}

	f_print(fout, "#ifndef _%s\n#define _%s\n\n", guard,
	    guard);

	f_print(fout, "#define RPCGEN_VERSION\t%s\n\n", RPCGEN_VERSION);
	f_print(fout, "#include <rpc/rpc.h>\n\n");

	tell = ftell(fout);
	/* print data definitions */
	while ((def = get_definition()) != NULL) {
		print_datadef(def);
	}

	/* print function declarations.  Do this after data definitions
	 * because they might be used as arguments for functions */
	did = 0;
	for (l = defined; l != NULL; l = l->next) {
		print_funcdef(l->val, &did);
	}
	print_funcend(did);

	for (l = defined; l != NULL; l = l->next) {
		print_progdef(l->val);
	}

	if (extend && tell == ftell(fout)) {
		(void) unlink(outfilename);
	} else
		if (tblflag) {
			f_print(fout, rpcgen_table_dcl);
		}
	f_print(fout, "\n#endif /* !_%s */\n", guard);

	free(guard);
}

/*
 * Compile into an RPC service
 */
static void
s_output(int argc, char *argv[], char *infile,
	 const char *define, int extend, const char *outfile, int nomain,
	 int netflag)
{
	char   *include;
	definition *def;
	int     foundprogram = 0;
	const char *outfilename;

	open_input(infile, define);
	outfilename = extend ? extendfile(infile, outfile) : outfile;
	open_output(infile, outfilename);
	add_warning();
	if (infile && (include = extendfile(infile, ".h"))) {
		f_print(fout, "#include \"%s\"\n", include);
		free(include);
	} else
		f_print(fout, "#include <rpc/rpc.h>\n");

	f_print(fout, "#include <sys/ioctl.h>\n");
	f_print(fout, "#include <fcntl.h>\n");
	f_print(fout, "#include <stdio.h>\n");
	f_print(fout, "#include <err.h>\n");
	f_print(fout, "#include <stdlib.h>\n");
	f_print(fout, "#include <unistd.h>\n");
	f_print(fout, "#include <rpc/pmap_clnt.h>\n");
	f_print(fout, "#include <string.h>\n");
	f_print(fout, "#include <netdb.h>\n");
	if (strcmp(svcclosetime, "-1") == 0)
		indefinitewait = 1;
	else
		if (strcmp(svcclosetime, "0") == 0)
			exitnow = 1;
		else
			if (inetdflag || pmflag) {
				f_print(fout, "#include <signal.h>\n");
				timerflag = 1;
			}
	if (!tirpcflag && inetdflag)
		f_print(fout, "#include <sys/ttycom.h>\n");
	if (inetdflag || pmflag) {
		f_print(fout, "#ifdef __cplusplus\n");
		f_print(fout, "#include <sysent.h>\n");
		f_print(fout, "#endif /* __cplusplus */\n");
	}
	if (tirpcflag)
		f_print(fout, "#include <sys/types.h>\n");

	f_print(fout, "#include <memory.h>\n");

	if (inetdflag || !tirpcflag) {
		f_print(fout, "#include <sys/socket.h>\n");
		f_print(fout, "#include <netinet/in.h>\n");
	}
	if ((netflag || pmflag) && tirpcflag) {
		f_print(fout, "#include <netconfig.h>\n");
	}
	if ( /* timerflag && */ tirpcflag)
		f_print(fout, "#include <sys/resource.h>\n");
	if (logflag || inetdflag || pmflag)
		f_print(fout, "#include <syslog.h>\n");

	f_print(fout, "\n#define SIG_PF void(*)(int)\n");

	f_print(fout, "\n#ifdef DEBUG\n#define RPC_SVC_FG\n#endif\n");
	if (timerflag)
		f_print(fout, "\n#define _RPCSVC_CLOSEDOWN %s\n", svcclosetime);
	while ((def = get_definition()) != NULL) {
		foundprogram |= (def->def_kind == DEF_PROGRAM);
	}
	if (extend && !foundprogram) {
		(void) unlink(outfilename);
		return;
	}
	if (callerflag)		/* EVAS */
		f_print(fout, "\nstatic SVCXPRT *caller;\n");	/* EVAS */
	write_most(infile, netflag, nomain);
	if (!nomain) {
		if (!do_registers(argc, argv)) {
			if (outfilename)
				(void) unlink(outfilename);
			usage();
		}
		write_rest();
	}
}
/*
 * generate client side stubs
 */
static void
l_output(const char *infile, const char *define, int extend,
	 const char *outfile)
{
	char   *include;
	definition *def;
	int     foundprogram = 0;
	const char *outfilename;

	open_input(infile, define);
	outfilename = extend ? extendfile(infile, outfile) : outfile;
	open_output(infile, outfilename);
	add_warning();
	f_print(fout, "#include <memory.h>\n");
	if (infile && (include = extendfile(infile, ".h"))) {
		f_print(fout, "#include \"%s\"\n", include);
		free(include);
	} else
		f_print(fout, "#include <rpc/rpc.h>\n");
	while ((def = get_definition()) != NULL) {
		foundprogram |= (def->def_kind == DEF_PROGRAM);
	}
	if (extend && !foundprogram) {
		(void) unlink(outfilename);
		return;
	}
	write_stubs();
}
/*
 * generate the dispatch table
 */
static void
t_output(const char *infile, const char *define, int extend,
	 const char *outfile)
{
	definition *def;
	int     foundprogram = 0;
	const char *outfilename;

	open_input(infile, define);
	outfilename = extend ? extendfile(infile, outfile) : outfile;
	open_output(infile, outfilename);
	add_warning();
	while ((def = get_definition()) != NULL) {
		foundprogram |= (def->def_kind == DEF_PROGRAM);
	}
	if (extend && !foundprogram) {
		(void) unlink(outfilename);
		return;
	}
	write_tables();
}
/* sample routine for the server template */
static void
svc_output(const char *infile, const char *define, int extend,
	   const char *outfile)
{
	definition *def;
	char   *include;
	const char *outfilename;
	long    tell;

	open_input(infile, define);
	outfilename = extend ? extendfile(infile, outfile) : outfile;
	checkfiles(infile, outfilename);	/* check if outfile already
						 * exists. if so, print an
						 * error message and exit */
	open_output(infile, outfilename);
	add_sample_msg();

	if (infile && (include = extendfile(infile, ".h"))) {
		f_print(fout, "#include \"%s\"\n", include);
		free(include);
	} else
		f_print(fout, "#include <rpc/rpc.h>\n");

	tell = ftell(fout);
	while ((def = get_definition()) != NULL) {
		write_sample_svc(def);
	}
	if (extend && tell == ftell(fout)) {
		(void) unlink(outfilename);
	}
}


/* sample main routine for client */
static void
clnt_output(const char *infile, const char *define, int extend,
	    const char *outfile)
{
	definition *def;
	char   *include;
	const char *outfilename;
	long    tell;
	int     has_program = 0;

	open_input(infile, define);
	outfilename = extend ? extendfile(infile, outfile) : outfile;
	checkfiles(infile, outfilename);	/* check if outfile already
						 * exists. if so, print an
						 * error message and exit */

	open_output(infile, outfilename);
	add_sample_msg();
	f_print(fout, "#include <stdio.h>\n");
	f_print(fout, "#include <err.h>\n");
	if (infile && (include = extendfile(infile, ".h"))) {
		f_print(fout, "#include \"%s\"\n", include);
		free(include);
	} else
		f_print(fout, "#include <rpc/rpc.h>\n");
	tell = ftell(fout);
	while ((def = get_definition()) != NULL) {
		has_program += write_sample_clnt(def);
	}

	if (has_program)
		write_sample_clnt_main();

	if (extend && tell == ftell(fout)) {
		(void) unlink(outfilename);
	}
}
/*
 * Perform registrations for service output
 * Return 0 if failed; 1 otherwise.
 */
static int
do_registers(int argc, char *argv[])
{
	int     i;

	if (inetdflag || !tirpcflag) {
		for (i = 1; i < argc; i++) {
			if (streq(argv[i], "-s")) {
				if (!check_nettype(argv[i + 1], valid_i_nettypes))
					return 0;
				write_inetd_register(argv[i + 1]);
				i++;
			}
		}
	} else {
		for (i = 1; i < argc; i++)
			if (streq(argv[i], "-s")) {
				if (!check_nettype(argv[i + 1], valid_ti_nettypes))
					return 0;
				write_nettype_register(argv[i + 1]);
				i++;
			} else
				if (streq(argv[i], "-n")) {
					write_netid_register(argv[i + 1]);
					i++;
				}
	}
	return 1;
}
/*
 * Add another argument to the arg list
 */
static void
addarg(const char *cp)
{
	if (argcount >= ARGLISTLEN) {
		errx(EXIT_FAILURE, "Internal error: too many defines");
		/* NOTREACHED */
	}
	arglist[argcount++] = cp;

}

static void
putarg(int pwhere, const char *cp)
{
	if (pwhere >= ARGLISTLEN) {
		errx(EXIT_FAILURE, "Internal error: arglist coding error");
		/* NOTREACHED */
	}
	arglist[pwhere] = cp;

}
/*
 * if input file is stdin and an output file is specified then complain
 * if the file already exists. Otherwise the file may get overwritten
 * If input file does not exist, exit with an error
 */

static void
checkfiles(const char *infile, const char *outfile)
{

	struct stat buf;

	if (infile)		/* infile ! = NULL */
		if (stat(infile, &buf) < 0) {
			err(EXIT_FAILURE, "Can't stat `%s'", infile);
		};
#if 0
	if (outfile) {
		if (stat(outfile, &buf) < 0)
			return;	/* file does not exist */
		else {
			errx(EXIT_FAILURE,
			    "`%s' already exists and would be overwritten",
			    outfile);
		}
	}
#endif
}
/*
 * Parse command line arguments
 */
static int
parseargs(int argc, char *argv[], struct commandline *cmd)
{
	int     i;
	int     j;
	int     c;
	char    flag[1 << CHAR_BIT];
	int     nflags;

	cmdname = argv[0];
	cmd->infile = cmd->outfile = NULL;
	if (argc < 2) {
		return (0);
	}
	allfiles = 0;
	flag['c'] = 0;
	flag['h'] = 0;
	flag['l'] = 0;
	flag['m'] = 0;
	flag['o'] = 0;
	flag['s'] = 0;
	flag['n'] = 0;
	flag['t'] = 0;
	flag['S'] = 0;
	flag['C'] = 0;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			if (cmd->infile) {
				f_print(stderr, "Cannot specify more than one input file!\n");

				return (0);
			}
			cmd->infile = argv[i];
		} else {
			for (j = 1; argv[i][j] != 0; j++) {
				c = argv[i][j];
				switch (c) {
				case 'A':
					callerflag = 1;
					break;
				case 'a':
					allfiles = 1;
					break;
				case 'B':
					BSDflag = 1;
					break;
				case 'c':
				case 'h':
				case 'l':
				case 'm':
				case 't':
					if (flag[c]) {
						return (0);
					}
					flag[c] = 1;
					break;
				case 'S':
					/* sample flag: Ss or Sc. Ss means set
					 * flag['S']; Sc means set flag['C']; */
					c = argv[i][++j];	/* get next char */
					if (c == 's')
						c = 'S';
					else
						if (c == 'c')
							c = 'C';
						else
							return (0);

					if (flag[c]) {
						return (0);
					}
					flag[c] = 1;
					break;
				case 'C':	/* deprecated ANSI C syntax */
					break;

				case 'b':	/* turn TIRPC flag off for
						 * generating backward
						 * compatible */
					tirpcflag = 0;
					break;

				case 'I':
					inetdflag = 1;
					break;
				case 'M':
					Mflag = 1;
					break;
				case 'N':
					newstyle = 1;
					break;
				case 'L':
					logflag = 1;
					break;
				case 'K':
					if (++i == argc) {
						return (0);
					}
					svcclosetime = argv[i];
					goto nextarg;
				case 'T':
					tblflag = 1;
					break;
				case 'i':
					if (++i == argc) {
						return (0);
					}
					doinline = atoi(argv[i]);
					goto nextarg;
				case 'n':
				case 'o':
				case 's':
					if (argv[i][j - 1] != '-' ||
					    argv[i][j + 1] != 0) {
						return (0);
					}
					flag[c] = 1;
					if (++i == argc) {
						return (0);
					}
					if (c == 's') {
						if (!streq(argv[i], "udp") &&
						    !streq(argv[i], "tcp")) {
							return (0);
						}
					} else
						if (c == 'o') {
							if (cmd->outfile) {
								return (0);
							}
							cmd->outfile = argv[i];
						}
					goto nextarg;
				case 'D':
					if (argv[i][j - 1] != '-') {
						return (0);
					}
					(void) addarg(argv[i]);
					goto nextarg;
				case 'Y':
					if (++i == argc) {
						return (0);
					}
					(void) strlcpy(pathbuf, argv[i],
					    sizeof(pathbuf));
					(void) strlcat(pathbuf, "/cpp",
					    sizeof(pathbuf));
					CPP = pathbuf;
					goto nextarg;

				case 'v':
					printf("version 1.0\n");
					exit(0);

				default:
					return (0);
				}
			}
	nextarg:
			;
		}
	}

	cmd->cflag = flag['c'];
	cmd->hflag = flag['h'];
	cmd->lflag = flag['l'];
	cmd->mflag = flag['m'];
	cmd->nflag = flag['n'];
	cmd->sflag = flag['s'];
	cmd->tflag = flag['t'];
	cmd->Ssflag = flag['S'];
	cmd->Scflag = flag['C'];

	if (tirpcflag) {
		pmflag = inetdflag ? 0 : 1;	/* pmflag or inetdflag is
						 * always TRUE */
		if ((inetdflag && cmd->nflag)) {	/* netid not allowed
							 * with inetdflag */
			f_print(stderr, "Cannot use netid flag with inetd flag!\n");
			return (0);
		}
	} else {		/* 4.1 mode */
		pmflag = 0;	/* set pmflag only in tirpcmode */
		inetdflag = 1;	/* inetdflag is TRUE by default */
		if (cmd->nflag) {	/* netid needs TIRPC */
			f_print(stderr, "Cannot use netid flag without TIRPC!\n");
			return (0);
		}
	}

	if (newstyle && (tblflag || cmd->tflag)) {
		f_print(stderr, "Cannot use table flags with newstyle!\n");
		return (0);
	}
	/* check no conflicts with file generation flags */
	nflags = cmd->cflag + cmd->hflag + cmd->lflag + cmd->mflag +
	    cmd->sflag + cmd->nflag + cmd->tflag + cmd->Ssflag + cmd->Scflag;

	if (nflags == 0) {
		if (cmd->outfile != NULL || cmd->infile == NULL) {
			return (0);
		}
	} else
		if (nflags > 1) {
			f_print(stderr, "Cannot have more than one file generation flag!\n");
			return (0);
		}
	return (1);
}

static void
usage(void)
{
	f_print(stderr, "usage:  %s infile\n", cmdname);
	f_print(stderr, "\t%s [-AaBbILMNTv] [-Dname[=value]] [-i size] [-K seconds] [-Y pathname] infile\n",
	    cmdname);
	f_print(stderr, "\t%s [-c | -h | -l | -m | -t | -Sc | -Ss] [-o outfile] [infile]\n",
	    cmdname);
	f_print(stderr, "\t%s [-s nettype] [-o outfile] [infile]\n", cmdname);
	f_print(stderr, "\t%s [-n netid] [-o outfile] [infile]\n", cmdname);
	options_usage();
	exit(1);
}

static void
options_usage(void)
{
	f_print(stderr, "options:\n");
	f_print(stderr, "-A\t\tgenerate svc_caller() function\n");
	f_print(stderr, "-a\t\tgenerate all files, including samples\n");
	f_print(stderr, "-B\t\tgenerate BSD c++ macros\n");
	f_print(stderr, "-b\t\tbackward compatibility mode (generates code for SunOS 4.1)\n");
	f_print(stderr, "-c\t\tgenerate XDR routines\n");
	f_print(stderr, "-Dname[=value]\tdefine a symbol (same as #define)\n");
	f_print(stderr, "-h\t\tgenerate header file\n");
	f_print(stderr, "-I\t\tgenerate code for inetd support in server (for SunOS 4.1)\n");
	f_print(stderr, "-i size\t\tsize at which to start generating inline code\n");
	f_print(stderr, "-K seconds\tserver exits after K seconds of inactivity\n");
	f_print(stderr, "-L\t\tserver errors will be printed to syslog\n");
	f_print(stderr, "-l\t\tgenerate client side stubs\n");
	f_print(stderr, "-M\t\tgenerate thread-safe stubs\n");
	f_print(stderr, "-m\t\tgenerate server side stubs\n");
	f_print(stderr, "-N\t\tsupports multiple arguments and call-by-value\n");
	f_print(stderr, "-n netid\tgenerate server code that supports named netid\n");
	f_print(stderr, "-o outfile\tname of the output file\n");
	f_print(stderr, "-s nettype\tgenerate server code that supports named nettype\n");
	f_print(stderr, "-Sc\t\tgenerate sample client code that uses remote procedures\n");
	f_print(stderr, "-Ss\t\tgenerate sample server code that defines remote procedures\n");
	f_print(stderr, "-T\t\tgenerate code to support RPC dispatch tables\n");
	f_print(stderr, "-t\t\tgenerate RPC dispatch table\n");
	f_print(stderr, "-v\t\tdisplay version number\n");
	f_print(stderr, "-Y path\t\tdirectory name to find C preprocessor (cpp)\n");

	exit(1);
}

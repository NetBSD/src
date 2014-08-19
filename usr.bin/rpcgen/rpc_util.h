/*	$NetBSD: rpc_util.h,v 1.5.64.1 2014/08/20 00:05:03 tls Exp $	*/
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

/*      @(#)rpc_util.h  1.5  90/08/29  (C) 1987 SMI   */

/*
 * rpc_util.h, Useful definitions for the RPC protocol compiler 
 */

#define alloc(size)		(void *)malloc((unsigned)(size))
#define ALLOC(object)   (object *) malloc(sizeof(object))

#define s_print	(void) sprintf
#define f_print (void) fprintf

struct list {
	definition *val;
	struct list *next;
};
typedef struct list list;

#define PUT 1
#define GET 2

/*
 * Global variables 
 */
#define MAXLINESIZE 1024
extern char curline[MAXLINESIZE];
extern char *where;
extern int linenum;

extern const char *infilename;
extern FILE *fout;
extern FILE *fin;

extern list *defined;


extern bas_type *typ_list_h;
extern bas_type *typ_list_t;

/*
 * All the option flags
 */
extern int inetdflag;
extern int pmflag;   
extern int tblflag;
extern int BSDflag;
extern int logflag;
extern int newstyle;
extern int Mflag;     /* multithread flag */
extern int tirpcflag; /* flag for generating tirpc code */
extern int doinline; /* if this is 0, then do not generate inline code */
extern int callerflag;

/*
 * Other flags related with inetd jumpstart.
 */
extern int indefinitewait;
extern int exitnow;
extern int timerflag;

extern int nonfatalerrors;

/*
 * rpc_util routines 
 */

#define STOREVAL(list,item)	\
	storeval(list,item)

#define FINDVAL(list,item,finder) \
	findval(list, item, finder)

void reinitialize(void);
int streq(const char *, const char *);
definition *findval(list *, const char *,
			 int (*)(definition *, const char *));
void storeval(list **, definition *);
const char *fixtype(const char *);
const char *stringfix(const char *);
void ptype(const char *, const char *, int);
int isvectordef(const char *, relation);
char *locase(const char *);
void pvname_svc(const char *, const char *);
void pvname(const char *, const char *);
void error(const char *) __dead;
void crash(void) __dead;
void record_open(const char *);
void expected1(tok_kind) __dead;
void expected2(tok_kind, tok_kind) __dead;
void expected3(tok_kind, tok_kind, tok_kind) __dead;
void tabify(FILE *, int);
char *make_argname(const char *, const char *);
void add_type(int, const char *);
bas_type *find_type(const char *);
/*
 * rpc_cout routines 
 */
void emit(definition *);
void emit_inline(declaration *, int);
void emit_single_in_line(declaration *, int, relation);
char *upcase(const char *);

/*
 * rpc_hout routines 
 */

void print_datadef(definition *);
void print_progdef(definition *);
void print_funcdef(definition *, int *);
void print_funcend(int);
void pxdrfuncdecl(const char *, int);
void pprocdef(proc_list *, version_list *, const char *, int);
void pdeclaration(const char *, declaration *, int, const char *);

/*
 * rpc_svcout routines 
 */
void write_most(char *, int, int);
void write_netid_register(const char *);
void write_nettype_register(const char *);
void write_rest(void);
void write_programs(const char *);
int nullproc(proc_list *);
void write_svc_aux(int);
void write_msg_out(void);
void write_inetd_register(const char *);

/*
 * rpc_clntout routines
 */
void write_stubs(void);
void printarglist(proc_list *, const char *, const char *, const char *);


/*
 * rpc_tblout routines
 */
void write_tables(void);

/*
 * rpc_sample routines
 */
void write_sample_svc(definition *);
int write_sample_clnt(definition *);
void add_sample_msg(void);
void write_sample_clnt_main(void);

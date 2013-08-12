/*	$NetBSD: rpc_util.h,v 1.7 2013/08/12 14:03:18 joerg Exp $	*/
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
extern int logflag;
extern int newstyle;
extern int Cflag;     /* C++ flag */
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

void reinitialize __P((void));
int streq __P((const char *, const char *));
definition *findval __P((list *, const char *,
			 int (*)(definition *, const char *)));
void storeval __P((list **, definition *));
const char *fixtype __P((const char *));
const char *stringfix __P((const char *));
void ptype __P((const char *, const char *, int));
int isvectordef __P((const char *, relation));
char *locase __P((const char *));
void pvname_svc __P((const char *, const char *));
void pvname __P((const char *, const char *));
void error(const char *) __dead;
void crash(void) __dead;
void record_open __P((const char *));
void expected1(tok_kind) __dead;
void expected2(tok_kind, tok_kind) __dead;
void expected3(tok_kind, tok_kind, tok_kind) __dead;
void tabify __P((FILE *, int));
char *make_argname __P((const char *, const char *));
void add_type __P((int, const char *));
bas_type *find_type __P((const char *));
/*
 * rpc_cout routines 
 */
void emit __P((definition *));
void emit_inline __P((declaration *, int));
void emit_single_in_line __P((declaration *, int, relation));
char *upcase __P((const char *));

/*
 * rpc_hout routines 
 */

void print_datadef __P((definition *));
void print_funcdef __P((definition *));
void pxdrfuncdecl __P((const char *, int));
void pprocdef __P((proc_list *, version_list *, const char *, int, int));
void pdeclaration __P((const char *, declaration *, int, const char *));

/*
 * rpc_svcout routines 
 */
void write_most __P((char *, int, int));
void write_netid_register __P((const char *));
void write_nettype_register __P((const char *));
void write_rest __P((void));
void write_programs __P((const char *));
int nullproc __P((proc_list *));
void write_svc_aux __P((int));
void write_msg_out __P((void));
void write_inetd_register __P((const char *));

/*
 * rpc_clntout routines
 */
void write_stubs __P((void));
void printarglist __P((proc_list *, const char *, const char *, const char *));


/*
 * rpc_tblout routines
 */
void write_tables __P((void));

/*
 * rpc_sample routines
 */
void write_sample_svc __P((definition *));
int write_sample_clnt __P((definition *));
void add_sample_msg __P((void));
void write_sample_clnt_main __P((void));

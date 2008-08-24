/*	$Id: fio.h,v 1.1.1.1 2008/08/24 05:34:47 gmcgarry Exp $	*/
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code and documentation must retain the above
 * copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditionsand the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 * 	This product includes software developed or owned by Caldera
 *	International, Inc.
 * Neither the name of Caldera International, Inc. nor the names of other
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OFLIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
typedef long ftnint;
typedef ftnint flag;
typedef long ftnlen;
/*external read, write*/
typedef struct
{	flag cierr;
	ftnint ciunit;
	flag ciend;
	char *cifmt;
	ftnint cirec;
} cilist;
/*internal read, write*/
typedef struct
{	flag icierr;
	char *iciunit;
	flag iciend;
	char *icifmt;
	ftnint icirlen;
	ftnint icirnum;
} icilist;
/*open*/
typedef struct
{	flag oerr;
	ftnint ounit;
	char *ofnm;
	ftnlen ofnmlen;
	char *osta;
	char *oacc;
	char *ofm;
	ftnint orl;
	char *oblnk;
} olist;
/*close*/
typedef struct
{	flag cerr;
	ftnint cunit;
	char *csta;
} cllist;
/*rewind, backspace, endfile*/
typedef struct
{	flag aerr;
	ftnint aunit;
} alist;
/*units*/
typedef struct
{	FILE *ufd;	/*0=unconnected*/
	char *ufnm;
	long uinode;
	int url;	/*0=sequential*/
	flag useek;	/*true=can backspace, use dir, ...*/
	flag ufmt;
	flag uprnt;
	flag ublnk;
	flag uend;
	flag uwrt;	/*last io was write*/
	flag uscrtch;
} unit;
typedef struct
{	flag inerr;
	ftnint inunit;
	char *infile;
	ftnlen infilen;
	ftnint	*inex;	/*parameters in standard's order*/
	ftnint	*inopen;
	ftnint	*innum;
	ftnint	*innamed;
	char	*inname;
	ftnlen	innamlen;
	char	*inacc;
	ftnlen	inacclen;
	char	*inseq;
	ftnlen	inseqlen;
	char 	*indir;
	ftnlen	indirlen;
	char	*infmt;
	ftnlen	infmtlen;
	char	*inform;
	ftnint	informlen;
	char	*inunf;
	ftnlen	inunflen;
	ftnint	*inrecl;
	ftnint	*innrec;
	char	*inblank;
	ftnlen	inblanklen;
} inlist;

extern int errno;
extern flag init;
extern cilist *elist;	/*active external io list*/
extern flag reading,external,sequential,formatted;
extern int (*getn)(void),(*putn)(int);	/*for formatted io*/
extern FILE *cf;	/*current file*/
extern unit *curunit;	/*current unit*/
extern unit units[];
#define err(f,n,s) {if(f) errno= n; else fatal(n,s); return(n);}

/*Table sizes*/
#define MXUNIT 10

extern int recpos;	/*position in current record*/

#define WRITE	1
#define READ	2
#define SEQ	3
#define DIR	4
#define FMT	5
#define UNF	6
#define EXT	7
#define INT	8

/* forward decl's */
struct syl;

/* function prototypes */
void fatal(int n, char *s);
int t_runc(unit *b);
int nowreading(unit *x);
int f_back(alist *a);
int rd_ed(struct syl *p, void *ptr, ftnlen len);
int rd_ned(struct syl *p, char *ptr);
int w_ed(struct syl *p, void *ptr, ftnlen len);
int w_ned(struct syl *p, char *ptr);
int s_rdfe(cilist *a);
void f_init(void);
int pars_f(char *);
void fmt_bg(void);
int s_wdfe(cilist *a);
int e_rdfe(void);
int e_wdfe(void);
int nowwriting(unit *);
int en_fio(void);
int do_fio(ftnint *number, char *ptr, ftnlen len);
int fk_open(int rd,int seq,int fmt, ftnint n);
int s_rdue(cilist *a);
int s_wdue(cilist *a);
int c_due(cilist *a, int flag);
int e_rdue(void);
int e_wdue(void);
int z_getc(void);
int z_putc(int);
int s_rsfi(icilist *a);
int s_wsfi(icilist *a);
int e_rsfi(void);
int e_wsfi(void);
int f_inqu(inlist *a);
void g_char(char *a, ftnlen alen, char *b);
void b_char(char *a, char *b, ftnlen blen);
int inode(char *a);
void setcilist(cilist *x, int u, char *fmt,int rec,int xerr,int end);
void setolist(olist *, int, char *, char *, char *, int, char *, int);
void stcllist(cllist *x, int xunit, char *stat, int cerr);
void setalist(alist *x, int xunit, int aerr);
int f_rew(alist *a);
int s_rsfe(cilist *a);
int c_sfe(cilist *a, int flag);
int s_rsue(cilist *a);
int s_wsue(cilist *a);
int e_rsue(void);
int e_wsue(void);
int do_uio(ftnint *number, char *ptr, ftnlen len);
int s_wsfe(cilist *a);
void pr_put(int);
int e_rsfe(void);
int e_wsfe(void);
int c_le(cilist *a, int flag);
int e_wsle(void);
int s_wsle(cilist *a);
int wrt_L(ftnint *n, int len);
int s_rsle(cilist *a);
int e_rsle(void);
int f_open(olist *a);
int f_clos(cllist *a);
void f_exit(void);
void flush_(void);
int fullpath(char *a,char *b, int errflag);
int f_end(alist *);
char *icvt(long value,int *ndigit,int *sign);


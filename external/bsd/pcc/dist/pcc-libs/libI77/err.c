/*	Id: err.c,v 1.3 2008/03/01 13:44:12 ragge Exp 	*/	
/*	$NetBSD: err.c,v 1.1.1.2 2010/06/03 18:58:11 plunky Exp $	*/
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
#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "fio.h"

#define STR(x) (x==NULL?"":x)

/*global definitions*/
unit units[MXUNIT];	/*unit table*/
flag init;	/*0 on entry, 1 after initializations*/
cilist *elist;	/*active external io list*/
flag reading;	/*1 if reading, 0 if writing*/
flag cplus,cblank;
char *fmtbuf;
flag external;	/*1 if external io, 0 if internal */
int (*doed)(struct syl *p, void *ptr, ftnlen len);
int (*doend)(void),(*donewrec)(void),(*dorevert)(void);
flag sequential;	/*1 if sequential io, 0 if direct*/
flag formatted;	/*1 if formatted io, 0 if unformatted*/
int (*getn)(void),(*putn)(int);	/*for formatted io*/
FILE *cf;	/*current file*/
unit *curunit;	/*current unit*/
int recpos;	/*place in current record*/
int cursor,scale;

int canseek(FILE *f);

/*error messages*/
char *F_err[] =
{
	"error in format",
	"illegal unit number",
	"formatted io not allowed",
	"unformatted io not allowed",
	"direct io not allowed",
	"sequential io not allowed",
	"can't backspace file",
	"null file name",
	"can't stat file",
	"unit not connected",
	"off end of record",
	"truncation failed in endfile",
	"incomprehensible list input",
	"out of free space",
	"unit not connected",
	"read unexpected character",
	"blank logical input field",
};
#define MAXERR (sizeof(F_err)/sizeof(char *)+100)

void
fatal(int n, char *s)
{
	if(n<100 && n>=0) perror(s); /*SYSDEP*/
	else if(n>=(int)MAXERR)
	{	fprintf(stderr,"%s: illegal error number %d\n",s,n);
	}
	else if(n<0) fprintf(stderr,"%s: end of file %d\n",s,n);
	else
		fprintf(stderr,"%s: %s\n",s,F_err[n-100]);
	fprintf(stderr,"apparent state: unit %d named %s\n",curunit-units,
		STR(curunit->ufnm));
	fprintf(stderr,"last format: %s\n",STR(fmtbuf));
	fprintf(stderr,"lately %s %s %s %s IO\n",reading?"reading":"writing",
		sequential?"sequential":"direct",formatted?"formatted":"unformatted",
		external?"external":"internal");
	abort();
}

/*initialization routine*/
void
f_init()
{	unit *p;
	init=1;
	p= &units[0];
	p->ufd=stderr;
	p->useek=canseek(stderr);
	p->ufmt=1;
	p->uwrt=1;
	p = &units[5];
	p->ufd=stdin;
	p->useek=canseek(stdin);
	p->ufmt=1;
	p->uwrt=0;
	p= &units[6];
	p->ufd=stdout;
	p->useek=canseek(stdout);
	p->ufmt=1;
	p->uwrt=1;
}

int
canseek(FILE *f) /*SYSDEP*/
{	struct stat x;
	fstat(fileno(f),&x);
	if(x.st_nlink > 0 /*pipe*/ && !isatty(fileno(f)))
	{
		return(1);
	}
	return(0);
}

int
nowreading(unit *x)
{
	long loc;
	x->uwrt=0;
	loc=ftell(x->ufd);
	freopen(x->ufnm,"r",x->ufd);
	return fseek(x->ufd,loc,0);
}

int
nowwriting(x) unit *x;
{
	long loc;
	loc=ftell(x->ufd);
	x->uwrt=1;
	freopen(x->ufnm,"a",x->ufd);
	return fseek(x->ufd,loc,0);
}

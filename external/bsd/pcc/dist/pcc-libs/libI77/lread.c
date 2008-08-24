/*	$Id: lread.c,v 1.1.1.1 2008/08/24 05:34:47 gmcgarry Exp $	*/
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
#include <stdlib.h>

#include "fio.h"
#include "fmt.h"
#include "lio.h"
#include "ctype.h"

extern char *fmtbuf;
int (*lioproc)(ftnint *number,flex *ptr,ftnlen len,ftnint type);

static int rd_int(double *x);
int l_R(void);
int l_C(void);
int l_L(void);
int l_CHAR(void);
int t_getc(void);
int t_sep(void);

#define isblnk(x) (ltab[x+1]&B)
#define issep(x) (ltab[x+1]&SX)
#define isapos(x) (ltab[x+1]&AX)
#define isexp(x) (ltab[x+1]&EX)
#define SX 1
#define B 2
#define AX 4
#define EX 8
char ltab[128+1] =	/* offset one for EOF */
{	0,
	0,0,AX,0,0,0,0,0,0,0,B,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	SX|B,0,AX,0,0,0,0,0,0,0,0,0,SX,0,0,SX,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,EX,EX,0,0,0,0,0,0,0,0,0,0,
	AX,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,EX,EX,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

int l_first;

int
t_getc()
{	int ch;
	if(curunit->uend) return(EOF);
	if((ch=getc(cf))!=EOF) return(ch);
	if(feof(cf)) curunit->uend = 1;
	return(EOF);
}

int
e_rsle()
{
	int ch;
	if(curunit->uend) return(0);
	while((ch=t_getc())!='\n' && ch!=EOF);
	return(0);
}

flag lquit;
int lcount,ltype;
char *lchar;
double lx,ly;
#define ERR(x) if((n=(x))) return(n)
#define GETC(x) ((x=t_getc()))

static int
l_read(ftnint *number,flex *ptr,ftnlen len,ftnint type)
{	int i,n,ch;
	double *yy;
	float *xx;
	for(i=0;i<*number;i++)
	{
		if(curunit->uend) err(elist->ciend, EOF, "list in")
		if(l_first)
		{	l_first=0;
			for(GETC(ch);isblnk(ch);GETC(ch));
			ungetc(ch,cf);
		}
		else if(lcount==0)
		{	ERR(t_sep());
			if(lquit) return(0);
		}
		switch((int)type)
		{
		case TYSHORT:
		case TYLONG:
		case TYREAL:
		case TYDREAL:
			ERR(l_R());
			break;
		case TYCOMPLEX:
		case TYDCOMPLEX:
			ERR(l_C());
			break;
		case TYLOGICAL:
			ERR(l_L());
			break;
		case TYCHAR:
			ERR(l_CHAR());
			break;
		}
		if(lquit) return(0);
		if(feof(cf)) err(elist->ciend,(EOF),"list in")
		else if(ferror(cf))
		{	clearerr(cf);
			err(elist->cierr,errno,"list in")
		}
		if(ltype==0) goto bump;
		switch((int)type)
		{
		case TYSHORT:
			ptr->flshort=lx;
			break;
		case TYLOGICAL:
		case TYLONG:
			ptr->flint=lx;
			break;
		case TYREAL:
			ptr->flreal=lx;
			break;
		case TYDREAL:
			ptr->fldouble=lx;
			break;
		case TYCOMPLEX:
			xx=(float *)ptr;
			*xx++ = lx;
			*xx = ly;
			break;
		case TYDCOMPLEX:
			yy=(double *)ptr;
			*yy++ = lx;
			*yy = ly;
			break;
		case TYCHAR:
			b_char(lchar,(char *)ptr,len);
			break;
		}
	bump:
		if(lcount>0) lcount--;
		ptr = (flex *)((char *)ptr + len);
	}
	return(0);
}

int
l_R()
{	double a,b,c,d;
	int i,ch,sign=0,da,db,dc;
	a=b=c=d=0;
	da=db=dc=0;
	if(lcount>0) return(0);
	ltype=0;
	for(GETC(ch);isblnk(ch);GETC(ch));
	if(ch==',')
	{	lcount=1;
		return(0);
	}
	if(ch=='/')
	{	lquit=1;
		return(0);
	}
	ungetc(ch,cf);
	da=rd_int(&a);
	if(da== -1) sign=da;
	if(GETC(ch)!='*')
	{	ungetc(ch,cf);
		db=1;
		b=a;
		a=1;
	}
	else
		db=rd_int(&b);
	if(GETC(ch)!='.')
	{	dc=c=0;
		ungetc(ch,cf);
	}
	else	dc=rd_int(&c);
	if(isexp(GETC(ch))) db=rd_int(&d);
	else
	{	ungetc(ch,cf);
		d=0;
	}
	lcount=a;
	if(!db && !dc)
		return(0);
	if(db && b<0)
	{	sign=1;
		b = -b;
	}
	for(i=0;i<dc;i++) c/=10;
	b=b+c;
	for(i=0;i<d;i++) b *= 10;
	for(i=0;i< -d;i++) b /= 10;
	if(sign) b = -b;
	ltype=TYLONG;
	lx=b;
	return(0);
}

int
rd_int(double *x)
{	int ch,sign=0,i;
	double y;
	i=0;
	y=0;
	if(GETC(ch)=='-') sign = -1;
	else if(ch=='+') sign=0;
	else ungetc(ch,cf);
	while(isdigit(GETC(ch)))
	{	i++;
		y=10*y+ch-'0';
	}
	ungetc(ch,cf);
	if(sign) y = -y;
	*x = y;
	return(y!=0?i:sign);
}

int
l_C()
{	int ch;
	if(lcount>0) return(0);
	ltype=0;
	for(GETC(ch);isblnk(ch);GETC(ch));
	if(ch==',')
	{	lcount=1;
		return(0);
	}
	if(ch=='/')
	{	lquit=1;
		return(0);
	}
	if(ch!='(')
	{	if(fscanf(cf,"%d",&lcount)!=1) {
			if(!feof(cf)) err(elist->cierr,112,"no rep")
			else err(elist->cierr,(EOF),"lread");
		}
		if(GETC(ch)!='*')
		{	ungetc(ch,cf);
			if(!feof(cf)) err(elist->cierr,112,"no star")
			else err(elist->cierr,(EOF),"lread");
		}
		if(GETC(ch)!='(')
		{	ungetc(ch,cf);
			return(0);
		}
	}
	lcount = 1;
	ltype=TYLONG;
	fscanf(cf,"%lf",&lx);
	while(isblnk(GETC(ch)));
	if(ch!=',')
	{	ungetc(ch,cf);
		err(elist->cierr,112,"no comma");
	}
	while(isblnk(GETC(ch)));
	ungetc(ch,cf);
	fscanf(cf,"%lf",&ly);
	while(isblnk(GETC(ch)));
	if(ch!=')') err(elist->cierr,112,"no )");
	while(isblnk(GETC(ch)));
	ungetc(ch,cf);
	return(0);
}

int
l_L()
{
	int ch;
	if(lcount>0) return(0);
	ltype=0;
	while(isblnk(GETC(ch)));
	if(ch==',')
	{	lcount=1;
		return(0);
	}
	if(ch=='/')
	{	lquit=1;
		return(0);
	}
	if(isdigit(ch))
	{	ungetc(ch,cf);
		fscanf(cf,"%d",&lcount);
		if(GETC(ch)!='*') {
			if(!feof(cf)) err(elist->cierr,112,"no star")
			else err(elist->cierr,(EOF),"lread");
		}
	}
	else	ungetc(ch,cf);
	if(GETC(ch)=='.') GETC(ch);
	switch(ch)
	{
	case 't':
	case 'T':
		lx=1;
		break;
	case 'f':
	case 'F':
		lx=0;
		break;
	default:
		if(isblnk(ch) || issep(ch) || ch==EOF)
		{	ungetc(ch,cf);
			return(0);
		}
		else	err(elist->cierr,112,"logical");
	}
	ltype=TYLONG;
	while(!issep(GETC(ch)) && ch!='\n' && ch!=EOF);
	return(0);
}
#define BUFSIZE	128

int
l_CHAR()
{	int ch,size,i;
	char quote,*p;
	if(lcount>0) return(0);
	ltype=0;

	while(isblnk(GETC(ch)));
	if(ch==',')
	{	lcount=1;
		return(0);
	}
	if(ch=='/')
	{	lquit=1;
		return(0);
	}
	if(isdigit(ch))
	{	ungetc(ch,cf);
		fscanf(cf,"%d",&lcount);
		if(GETC(ch)!='*') err(elist->cierr,112,"no star");
	}
	else	ungetc(ch,cf);
	if(GETC(ch)=='\'' || ch=='"') quote=ch;
	else if(isblnk(ch) || issep(ch) || ch==EOF)
	{	ungetc(ch,cf);
		return(0);
	}
	else err(elist->cierr,112,"no quote");
	ltype=TYCHAR;
	if(lchar!=NULL) free(lchar);
	size=BUFSIZE;
	p=lchar=(char *)malloc(size);
	if(lchar==NULL) err(elist->cierr,113,"no space");
	for(i=0;;)
	{	while(GETC(ch)!=quote && ch!='\n'
			&& ch!=EOF && ++i<size) *p++ = ch;
		if(i==size)
		{
		newone:
			lchar=(char *)realloc(lchar, size += BUFSIZE);
			p=lchar+i-1;
			*p++ = ch;
		}
		else if(ch==EOF) return(EOF);
		else if(ch=='\n')
		{	if(*(p-1) != '\\') continue;
			i--;
			p--;
			if(++i<size) *p++ = ch;
			else goto newone;
		}
		else if(GETC(ch)==quote)
		{	if(++i<size) *p++ = ch;
			else goto newone;
		}
		else
		{	ungetc(ch,cf);
			*p++ = 0;
			return(0);
		}
	}
}

int
s_rsle(cilist *a)
{
	int n;
	if(!init) f_init();
	if((n=c_le(a,READ))) return(n);
	reading=1;
	external=1;
	formatted=1;
	l_first=1;
	lioproc = l_read;
	lcount = 0;
	if(curunit->uwrt)
		return(nowreading(curunit));
	else	return(0);
}

int
t_sep()
{
	int ch;
	for(GETC(ch);isblnk(ch);GETC(ch));
	if(ch == EOF) {
		if(feof(cf)) return(EOF);
		else return(errno);
	}
	if(ch=='/') {
		lquit=1;
		return(0);
	}
	if(ch==',') for(GETC(ch);isblnk(ch);GETC(ch));
	ungetc(ch,cf);
	return(0);
}

int
c_le(cilist *a, int flag)
{
	fmtbuf="list io";
	if(a->ciunit>=MXUNIT || a->ciunit<0)
		err(a->cierr,101,"stler");
	scale=recpos=0;
	elist=a;
	curunit = &units[a->ciunit];
	if(curunit->ufd==NULL && fk_open(flag,SEQ,FMT,a->ciunit))
		err(a->cierr,102,"lio");
	cf=curunit->ufd;
	if(!curunit->ufmt) err(a->cierr,103,"lio")
	return(0);
}

int
do_lio(ftnint *type,ftnint *number,flex *ptr,ftnlen len)
{
	return((*lioproc)(number,ptr,len,*type));
}

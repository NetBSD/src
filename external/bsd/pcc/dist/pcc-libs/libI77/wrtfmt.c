/*	Id: wrtfmt.c,v 1.3 2008/03/01 13:42:02 ragge Exp 	*/	
/*	$NetBSD: wrtfmt.c,v 1.1.1.2 2010/06/03 18:58:12 plunky Exp $	*/
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
#include <stdlib.h>

#include "fio.h"
#include "fmt.h"

int wrt_I(unint *n,int w,ftnlen len);
int wrt_IM(unint *n,int w,int m,ftnlen len);
int wrt_AP(int n);
int wrt_A(char *p,ftnlen len);
int wrt_AW(char *p, int w,ftnlen len);
int wrt_G(ufloat *p,int w,int d,int e,ftnlen len);
int wrt_H(int a,int b);

extern int cursor;
static int
mv_cur(void)
{	/*buggy, could move off front of record*/
	for(;cursor>0;cursor--) (*putn)(' ');
	if(cursor<0)
	{
		if(cursor+recpos<0) err(elist->cierr,110,"left off");
		if(curunit->useek) fseek(cf,(long)cursor,1);
		else err(elist->cierr,106,"fmt");
		cursor=0;
	}
	return(0);
}

int
w_ed(struct syl *p, void *ptr, ftnlen len)
{
	if(mv_cur()) return(mv_cur());
	switch(p->op)
	{
	default:
		fprintf(stderr,"w_ed, unexpected code: %d\n%s\n",
			p->op,fmtbuf);
		abort();
	case I:	return(wrt_I(ptr,p->p1,len));
	case IM:
		return(wrt_IM(ptr,p->p1,p->p2,len));
	case L:	return(wrt_L(ptr,p->p1));
	case A: return(wrt_A(ptr,len));
	case AW:
		return(wrt_AW(ptr,p->p1,len));
	case D:
	case E:
	case EE:
		return(wrt_E(ptr,p->p1,p->p2,p->p3,len));
	case G:
	case GE:
		return(wrt_G(ptr,p->p1,p->p2,p->p3,len));
	case F:	return(wrt_F(ptr,p->p1,p->p2,len));
	}
}

int
w_ned(struct syl *p, char *ptr)
{
	switch(p->op)
	{
	default: fprintf(stderr,"w_ned, unexpected code: %d\n%s\n",
			p->op,fmtbuf);
		abort();
	case SLASH:
		return((*donewrec)());
	case T: cursor = p->p1-recpos;
		return(1);
	case TL: cursor -= p->p1;
		return(1);
	case TR:
	case X:
		cursor += p->p1;
		return(1);
	case APOS:
		return(wrt_AP(p->p1));
	case H:
		return(wrt_H(p->p1,p->p2));
	}
}

int
wrt_I(unint *n,int w,ftnlen len)
{	int ndigit,sign,spare,i;
	long x;
	char *ans;
	if(len==sizeof(short)) x=n->is;
	else if(len == sizeof(char)) x = n->ic;
	else x=n->il;
	ans=icvt(x,&ndigit,&sign);
	spare=w-ndigit;
	if(sign || cplus) spare--;
	if(spare<0)
		for(i=0;i<len;i++) (*putn)('*');
	else
	{	for(i=0;i<spare;i++) (*putn)(' ');
		if(sign) (*putn)('-');
		else if(cplus) (*putn)('+');
		for(i=0;i<ndigit;i++) (*putn)(*ans++);
	}
	return(0);
}

int
wrt_IM(unint *n,int w,int m,ftnlen len)
{	int ndigit,sign,spare,i,xsign;
	long x;
	char *ans;
	if(sizeof(short)==len) x=n->is;
	else if(len == sizeof(char)) x = n->ic;
	else x=n->il;
	ans=icvt(x,&ndigit,&sign);
	if(sign || cplus) xsign=1;
	else xsign=0;
	if(ndigit+xsign>w || m+xsign>w)
	{	for(i=0;i<w;i++) (*putn)('*');
		return(0);
	}
	if(x==0 && m==0)
	{	for(i=0;i<w;i++) (*putn)(' ');
		return(0);
	}
	if(ndigit>=m)
		spare=w-ndigit-xsign;
	else
		spare=w-m-xsign;
	for(i=0;i<spare;i++) (*putn)(' ');
	if(sign) (*putn)('-');
	else if(cplus) (*putn)('+');
	for(i=0;i<m-ndigit;i++) (*putn)('0');
	for(i=0;i<ndigit;i++) (*putn)(*ans++);
	return(0);
}

int
wrt_AP(int n)
{	char *s,quote;
	if(mv_cur()) return(mv_cur());
	s=(char *)n;
	quote = *s++;
	for(;*s;s++)
	{	if(*s!=quote) (*putn)(*s);
		else if(*++s==quote) (*putn)(*s);
		else return(1);
	}
	return(1);
}

int
wrt_H(int a,int b)
{	char *s=(char *)b;
	if(mv_cur()) return(mv_cur());
	while(a--) (*putn)(*s++);
	return(1);
}

int
wrt_L(ftnint *n, int len)
{	int i;
	for(i=0;i<len-1;i++)
		(*putn)(' ');
	if(*n) (*putn)('t');
	else (*putn)('f');
	return(0);
}

int
wrt_A(char *p,ftnlen len)
{
	while(len-- > 0) (*putn)(*p++);
	return(0);
}

int
wrt_AW(char *p, int w,ftnlen len)
{
	while(w>len)
	{	w--;
		(*putn)(' ');
	}
	while(w-- > 0)
		(*putn)(*p++);
	return(0);
}

#define MXSTR 80
char nr[MXSTR];

/*
 * Trivial ecvt implementation.
 */
static char *
Xecvt(double value, int ndigit, int *decpt, int *sign)
{
	char fmt[10];
	char *w = nr;

	if (ndigit > 70)
		ndigit = 70;

	snprintf(fmt, 10, "%%# .%de", ndigit-1);
	snprintf(nr, MXSTR, fmt, value);
	*sign = (*w == '-' ? 1 : 0);
	w[2] = w[1];
	*decpt = atoi(&nr[ndigit+3]) + 1;
	nr[ndigit+2] = 0;
	return &nr[2];
}


int
wrt_E(ufloat *p,int w,int d,int e, ftnlen len)
{	char *s;
	int dp,sign,i,delta;

	if(scale>0) d++;
	s=Xecvt( (len==sizeof(float)?p->pf:p->pd) ,d,&dp,&sign);
	if(sign || cplus) delta=6;
	else delta=5;
	if(w<delta+d)
	{	for(i=0;i<w;i++) (*putn)('*');
		return(0);
	}
	for(i=0;i<w-(delta+d);i++) (*putn)(' ');
	if(sign) (*putn)('-');
	else if(cplus) (*putn)('+');
	if(scale<0 && scale > -d)
	{
		(*putn)('.');
		for(i=0;i<-scale;i++)
			(*putn)('0');
		for(i=0;i<d+scale;i++)
			(*putn)(*s++);
	}
	else if(scale>0 && scale<d+2)
	{	for(i=0;i<scale;i++)
			(*putn)(*s++);
		(*putn)('.');
		for(i=0;i<d-scale;i++)
			(*putn)(*s++);
	}
	else
	{	(*putn)('.');
		for(i=0;i<d;i++) (*putn)(*s++);
	}
	if(p->pf != 0) dp -= scale;
	else	dp = 0;
	if(dp < 100 && dp > -100) (*putn)('e');
	if(dp<0)
	{	(*putn)('-');
		dp = -dp;
	}
	else	(*putn)('+');
	if(e>=3 || dp >= 100)
	{	(*putn)(dp/100 + '0');
		dp = dp % 100;
	}
	if(e!=1) (*putn)(dp/10+'0');
	(*putn)(dp%10+'0');
	return(0);
}

int
wrt_G(ufloat *p,int w,int d,int e,ftnlen len)
{	double up = 1,x;
	int i,oldscale=scale,n,j;
	x= len==sizeof(float)?p->pf:p->pd;
	if(x < 0 ) x = -x;
	if(x<.1) return(wrt_E(p,w,d,e,len));
	for(i=0;i<=d;i++,up*=10)
	{	if(x>up) continue;
		scale=0;
		if(e==0) n=4;
		else	n=e+2;
		i=wrt_F(p,w-n,d-i,len);
		for(j=0;j<n;j++) (*putn)(' ');
		scale=oldscale;
		return(i);
	}
	return(wrt_E(p,w,d,e,len));
}

/*
 * Simple fcvt() implementation.
 */
static char *
Xfcvt(double value, int ndigit, int *decpt, int *sign)
{
	char fmt[10];
	char *w = nr;

	if (ndigit > 70)
		ndigit = 70;

	snprintf(fmt, 10, "%%# .%df", ndigit);
	snprintf(nr, MXSTR, fmt, value);
	*sign = (*w == '-' ? 1 : 0);
	if (w[1] == '0') {
		*decpt = 0;
		w+= 3;
	} else {
		for (w+= 1; *w && *w != '.'; w++)
			;
		*decpt = w - nr - 1;
		while (*w)
			*w = w[1], w++;
		w = &nr[1];
	}
	return w;
}


int
wrt_F(ufloat *p, int w,int d, ftnlen len)
{	int i,delta,dp,sign,n;
	double x;
	char *s;

	x= (len==sizeof(float)?p->pf:p->pd);
	if(scale)
	{	if(scale>0)
			for(i=0;i<scale;i++) x*=10;
		else	for(i=0;i<-scale;i++) x/=10;
	}
	s=Xfcvt(x,d,&dp,&sign);
	if(-dp>=d) sign=0;
	if(sign || cplus) delta=2;
	else delta=1;
	n= w - (d+delta+(dp>0?dp:0));
	if(n<0)
	{
		for(i=0;i<w;i++) PUT('*');
		return(0);
	}
	for(i=0;i<n;i++) PUT(' ');
	if(sign) PUT('-');
	else if(cplus) PUT('+');
	for(i=0;i<dp;i++) PUT(*s++);
	PUT('.');
	for(i=0;i< -dp && i<d;i++) PUT('0');
	for(;i<d;i++)
	{	if(*s) PUT(*s++);
		else PUT('0');
	}
	return(0);
}

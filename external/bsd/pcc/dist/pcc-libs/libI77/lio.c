/*	$Id: lio.c,v 1.1.1.1.2.2 2008/09/18 05:15:46 wrstuden Exp $	*/
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
#include <string.h>

#include "fio.h"
#include "fmt.h"
#include "lio.h"

static int l_write(ftnint *number, flex *ptr,ftnlen len, ftnint type);
static int t_putc(int);
void lwrt_I(ftnint n);
void lwrt_L(ftnint n);
void lwrt_A(char *p, ftnlen len);
void lwrt_F(double n);
void lwrt_C(double a,double b);

int
s_wsle(cilist *a)
{
	int n;
	if(!init) f_init();
	if((n=c_le(a,WRITE))) return(n);
	reading=0;
	external=1;
	formatted=1;
	putn = t_putc;
	lioproc = l_write;
	if(!curunit->uwrt)
		return(nowwriting(curunit));
	else	return(0);
}

int
e_wsle()
{
	t_putc('\n');
	recpos=0;
	return(0);
}

int
t_putc(c)
{
	recpos++;
	putc(c,cf);
	return 0;
}

void
lwrt_I(ftnint n)
{
	char buf[LINTW],*p;
	sprintf(buf," %ld",(long)n);
	if(recpos+strlen(buf)>=LINE)
	{	t_putc('\n');
		recpos=0;
	}
	for(p=buf;*p;t_putc(*p++));
}

void
lwrt_L(ftnint n)
{
	if(recpos+LLOGW>=LINE)
	{	t_putc('\n');
		recpos=0;
	}
	wrt_L(&n,LLOGW);
}

void
lwrt_A(char *p, ftnlen len)
{
	int i;
	if(recpos+len>=LINE)
	{
		t_putc('\n');
		recpos=0;
	}
	t_putc(' ');
	for(i=0;i<len;i++) t_putc(*p++);
}

void
lwrt_F(double n)
{
	ufloat uf;

	uf.pd = n;
	if(LLOW<=n && n<LHIGH)
	{
		if(recpos+LFW>=LINE)
		{
			t_putc('\n');
			recpos=0;
		}
		scale=0;
		wrt_F(&uf,LFW,LFD,(ftnlen)sizeof(uf));
	}
	else
	{
		if(recpos+LEW>=LINE)
		{	t_putc('\n');
			recpos=0;
		}
		wrt_E(&uf,LEW,LED,LEE,(ftnlen)sizeof(uf));
	}
}

void
lwrt_C(double a,double b)
{
	if(recpos+2*LFW+3>=LINE)
	{	t_putc('\n');
		recpos=0;
	}
	t_putc(' ');
	t_putc('(');
	lwrt_F(a);
	lwrt_F(b);
	t_putc(')');
}

int
l_write(ftnint *number, flex *ptr,ftnlen len, ftnint type)
{
	int i;
	ftnint x;
	double y,z;
	float *xx;
	double *yy;
	for(i=0;i< *number; i++)
	{
		switch((int)type)
		{
		default: fatal(204,"unknown type in lio");
		case TYSHORT: x=ptr->flshort;
			goto xint;
		case TYLONG: x=ptr->flint;
		xint: lwrt_I(x);
			break;
		case TYREAL: y=ptr->flreal;
			goto xfloat;
		case TYDREAL: y=ptr->fldouble;
		xfloat: lwrt_F(y);
			break;
		case TYCOMPLEX: xx= &(ptr->flreal);
			y = *xx++;
			z = *xx;
			goto xcomplex;
		case TYDCOMPLEX: yy = &(ptr->fldouble);
			y= *yy++;
			z = *yy;
		xcomplex: lwrt_C(y,z);
			break;
		case TYLOGICAL: lwrt_L(ptr->flint);
			break;
		case TYCHAR: lwrt_A((char *)ptr,len);
			break;
		}
		ptr = (flex *)((char *)ptr + len); /* XXX ??? */
	}
	return(0);
}

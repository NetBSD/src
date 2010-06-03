/*	Id: iio.c,v 1.3 2008/03/01 13:44:12 ragge Exp 	*/	
/*	$NetBSD: iio.c,v 1.1.1.2 2010/06/03 18:58:11 plunky Exp $	*/
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
#include "fio.h"
#include "fmt.h"
char *icptr,*icend;
icilist *svic;
static int y_err(void), z_wnew(void), z_rnew(void), c_si(icilist *);
int icnum,icpos;

int
z_getc()
{
	if(icptr >= icend) err(svic->iciend,(EOF),"endfile");
	if(icpos++ < svic->icirlen)
		return(*icptr++);
	else	err(svic->icierr,110,"recend");
}

int
z_putc(int c)
{
	if(icptr >= icend) err(svic->icierr,110,"inwrite");
	if(icpos++ < svic->icirlen)
		*icptr++ = c;
	else	err(svic->icierr,110,"recend");
	return(0);
}

int
z_rnew()
{
	icptr = svic->iciunit + (++icnum)*svic->icirlen;
	icpos = 0;
	return 0;
}

int
s_rsfi(icilist *a)
{	int n;
	if((n=c_si(a))) return(n);
	reading=1;
	doed=rd_ed;
	doned=rd_ned;
	getn=z_getc;
	dorevert = donewrec = y_err;
	doend = z_rnew;
	return(0);
}

int
s_wsfi(icilist *a)
{	int n;
	if((n=c_si(a))) return(n);
	reading=0;
	doed=w_ed;
	doned=w_ned;
	putn=z_putc;
	dorevert = donewrec = y_err;
	doend = z_wnew;
	return(0);
}

int
c_si(icilist *a)
{
	fmtbuf=a->icifmt;
	if(pars_f(fmtbuf)<0)
		err(a->icierr,100,"startint");
	fmt_bg();
	sequential=formatted=1;
	external=0;
	cblank=cplus=scale=0;
	svic=a;
	icnum=icpos=0;
	icptr=svic->iciunit;
	icend=icptr+svic->icirlen*svic->icirnum;
	return(0);
}

int
z_wnew()
{
	while(icpos++ < svic->icirlen)
		*icptr++ = ' ';
	icpos = 0;
	icnum++;
	return 0;
}

int
e_rsfi()
{	int n;
	n = en_fio();
	fmtbuf = NULL;
	return(n);
}

int
e_wsfi()
{
	int n;
	n = en_fio();
	fmtbuf = NULL;
	while(icpos++ < svic->icirlen)
		*icptr++ = ' ';
	return(n);
}

int
y_err()
{
	err(elist->cierr, 110, "iio");
}

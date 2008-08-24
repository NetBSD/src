/*	$Id: rsfe.c,v 1.1.1.1 2008/08/24 05:34:48 gmcgarry Exp $	*/
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
/* read sequential formatted external */
#include "fio.h"
#include "fmt.h"
static int x_getc(void), x_endp(void), x_rev(void);
static int xrd_SL(void);

int
s_rsfe(cilist *a) /* start */
{	int n;
	if(!init) f_init();
	if((n=c_sfe(a,READ))) return(n);
	reading=1;
	sequential=1;
	formatted=1;
	external=1;
	elist=a;
	cursor=recpos=0;
	scale=0;
	fmtbuf=a->cifmt;
	if(pars_f(fmtbuf)<0) err(a->cierr,100,"startio");
	curunit= &units[a->ciunit];
	cf=curunit->ufd;
	getn= x_getc;
	doed= rd_ed;
	doned= rd_ned;
	fmt_bg();
	doend=x_endp;
	donewrec=xrd_SL;
	dorevert=x_rev;
	cblank=curunit->ublnk;
	cplus=0;
	if(curunit->uwrt) nowreading(curunit);
	return(0);
}

int
xrd_SL()
{	int ch;
	if(!curunit->uend)
		while((ch=getc(cf))!='\n' && ch!=EOF);
	cursor=recpos=0;
	return(1);
}

int
x_getc()
{	int ch;
	if(curunit->uend) return(EOF);
	if((ch=getc(cf))!=EOF && ch!='\n')
	{	recpos++;
		return(ch);
	}
	if(ch=='\n')
	{	ungetc(ch,cf);
		return(ch);
	}
	if(feof(cf))
	{	errno=0;
		curunit->uend=1;
		return(-1);
	}
	return(-1);
}

int
x_endp()
{
	xrd_SL();
	return(0);
}

int
x_rev()
{
	xrd_SL();
	return(0);
}

/*	$Id: wsfe.c,v 1.1.1.1.2.2 2008/09/18 05:15:46 wrstuden Exp $	*/
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
/*write sequential formatted external*/
#include "fio.h"
#include "fmt.h"
static int x_putc(int);
static int xw_end(void),xw_rev(void),x_wSL(void);


int
s_wsfe(cilist *a)
{	int n;
	if(!init) f_init();
	if((n=c_sfe(a,WRITE))) return(n);
	reading=0;
	sequential=1;
	formatted=1;
	external=1;
	elist=a;
	cursor=recpos=0;
	scale=0;
	fmtbuf=a->cifmt;
	if(pars_f(fmtbuf)<0) err(a->cierr,100,"startio");
	curunit = &units[a->ciunit];
	cf=curunit->ufd;
	putn= x_putc;
	doed= w_ed;
	doned= w_ned;
	doend=xw_end;
	dorevert=xw_rev;
	donewrec=x_wSL;
	fmt_bg();
	cplus=0;
	cblank=curunit->ublnk;
	if(!curunit->uwrt) nowwriting(curunit);
	return(0);
}

int
x_putc(c)
{
	recpos++;
	putc(c,cf);
	return 0;
}

void
pr_put(c)
{	static flag new = 1;
	recpos++;
	if(c=='\n')
	{	new=1;
		putc(c,cf);
	}
	else if(new==1)
	{	new=0;
		if(c=='0') putc('\n',cf);
		else if(c=='1') putc('\f',cf);
	}
	else putc(c,cf);
}

int
x_wSL()
{
	recpos=0;
	(*putn)('\n');
	return(1);
}

int
xw_end()
{
	(*putn)('\n');
	return(0);
}

int
xw_rev()
{
	if(workdone) (*putn)('\n');
	return(workdone=0);
}

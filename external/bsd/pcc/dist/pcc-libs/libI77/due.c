/*	$Id: due.c,v 1.1.1.1.6.2 2008/10/19 22:40:27 haad Exp $	*/
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

int
s_rdue(cilist *a)
{
	int n;
	if((n=c_due(a,READ))) return(n);
	reading=1;
	if(curunit->uwrt) nowreading(curunit);
	return(0);
}

int
s_wdue(cilist *a)
{
	int n;
	if((n=c_due(a,WRITE))) return(n);
	reading=0;
	if(!curunit->uwrt) nowwriting(curunit);
	return(0);
}

int
c_due(cilist *a, int flag)
{
	if(!init) f_init();
	if(a->ciunit>=MXUNIT || a->ciunit<0)
		err(a->cierr,101,"startio");
	recpos=sequential=formatted=0;
	external=1;
	curunit = &units[a->ciunit];
	elist=a;
	if(curunit->ufd==NULL && fk_open(flag,DIR,UNF,a->ciunit) ) err(a->cierr,104,"due");
	cf=curunit->ufd;
	if(curunit->ufmt) err(a->cierr,102,"cdue")
	if(!curunit->useek) err(a->cierr,104,"cdue")
	if(curunit->ufd==NULL) err(a->cierr,114,"cdue")
	fseek(cf,(long)(a->cirec-1)*curunit->url,0);
	curunit->uend = 0;
	return(0);
}

int
e_rdue()
{
	if(curunit->url==1 || recpos==curunit->url)
		return(0);
	fseek(cf,(long)(curunit->url-recpos),1);
	if(ftell(cf)%curunit->url)
		err(elist->cierr,200,"syserr");
	return(0);
}

int
e_wdue()
{
	return(e_rdue());
}

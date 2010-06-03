/*	Id: backspace.c,v 1.3 2008/03/01 13:44:12 ragge Exp 	*/	
/*	$NetBSD: backspace.c,v 1.1.1.2 2010/06/03 18:58:10 plunky Exp $	*/
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
f_back(alist *a)
{
	unit *b;
	int n,i;
	long x;
	char buf[32];

	if(a->aunit >= MXUNIT || a->aunit < 0)
		err(a->aerr,101,"backspace")
	b= &units[a->aunit];
	if(b->useek==0) err(a->aerr,106,"backspace")
	if(b->ufd==NULL) err(a->aerr,114,"backspace")
	if(b->uend==1)
	{	b->uend=0;
		return(0);
	}
	if(b->uwrt)
	{	t_runc(b);
		nowreading(b);
	}
	if(b->url>0)
	{
		x=ftell(b->ufd);
		x /= b->url;
		x *= b->url;
		fseek(b->ufd,x,0);
		return(0);
	}
	if(b->ufmt==0)
	{	fseek(b->ufd,-(long)sizeof(int),1);
		fread((char *)&n,sizeof(int),1,b->ufd);
		fseek(b->ufd,-(long)n-2*sizeof(int),1);
		return(0);
	}
	for(;;)
	{
		x=ftell(b->ufd);
		if(x<sizeof(buf)) x=0;
		else x -= sizeof(buf);
		fseek(b->ufd,x,0);
		n=fread(buf,1,sizeof(buf),b->ufd);
		for(i=n-1;i>=0;i--)
		{
			if(buf[i]!='\n') continue;
			fseek(b->ufd,(long)(i-n),1);
			return(0);
		}
		if(x==0) return(0);
		else if(n==0) err(a->aerr,(EOF),"backspace")
		else err(a->aerr,errno,"backspace");
	}
}

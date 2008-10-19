/*	$Id: sue.c,v 1.1.1.1.6.2 2008/10/19 22:40:28 haad Exp $	*/
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
extern int reclen;
long recloc;

static int c_sue(cilist *a, int flag);

int
s_rsue(cilist *a)
{
	int n;
	if(!init) f_init();
	if((n=c_sue(a,READ))) return(n);
	reading=1;
	recpos=0;
	if(curunit->uwrt) nowreading(curunit);
	if(fread(&reclen,sizeof(int),1,cf)
		!= 1)
	{	if(feof(cf))
		{	curunit->uend = 1;
			err(a->ciend, EOF, "start");
		}
		clearerr(cf);
		err(a->cierr, errno, "start");
	}
	return(0);
}

int
s_wsue(cilist *a)
{
	int n;
	if(!init) f_init();
	if((n=c_sue(a,WRITE))) return(n);
	reading=0;
	reclen=0;
	if(!curunit->uwrt) nowwriting(curunit);
	recloc=ftell(cf);
	fseek(cf,(long)sizeof(int),1);
	return(0);
}

int
c_sue(cilist *a, int flag)
{
	if(a->ciunit >= MXUNIT || a->ciunit < 0)
		err(a->cierr,101,"startio");
	external=sequential=1;
	formatted=0;
	curunit = &units[a->ciunit];
	elist=a;
	if(curunit->ufd==NULL && fk_open(flag,SEQ,UNF,a->ciunit))
		err(a->cierr,114,"sue");
	cf=curunit->ufd;
	if(curunit->ufmt) err(a->cierr,103,"sue")
	if(!curunit->useek) err(a->cierr,103,"sue")
	return(0);
}

int
e_wsue()
{	long loc;
	fwrite(&reclen,sizeof(int),1,cf);
	loc=ftell(cf);
	fseek(cf,recloc,0);
	fwrite(&reclen,sizeof(int),1,cf);
	fseek(cf,loc,0);
	return(0);
}

int
e_rsue()
{
	fseek(cf,(long)(reclen-recpos+sizeof(int)),1);
	return(0);
}

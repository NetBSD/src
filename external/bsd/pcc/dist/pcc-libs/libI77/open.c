/*	$Id: open.c,v 1.1.1.1 2008/08/24 05:34:47 gmcgarry Exp $	*/
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

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fio.h"

static int isdev(char *s);
int canseek(FILE *f);


int
f_open(olist *a)
{	unit *b;
	int n;
	char buf[256];
	cllist x;
	if(a->ounit>=MXUNIT || a->ounit<0)
		err(a->oerr,101,"open")
	b= &units[a->ounit];
	if(b->ufd!=0) goto connected;
unconnected:
	b->url=a->orl;
	if(a->oblnk && *a->oblnk=='b') b->ublnk=1;
	else b->ublnk=0;
	if(a->ofm==0)
	{	if(b->url>0) b->ufmt=0;
		else b->ufmt=1;
	}
	else if(*a->ofm=='f') b->ufmt=1;
	else b->ufmt=0;
	if(a->osta==0) goto unknown;
	switch(*a->osta)
	{
	unknown:
	default:
	case 'o':
		if(a->ofnm==0) err(a->oerr,107,"open")
		g_char(a->ofnm,a->ofnmlen,buf);
		b->uscrtch=0;
		if(a->osta && *a->osta=='o' && access(buf,0))
			err(a->oerr,errno,"open")
	done:
		b->ufnm=(char *) calloc(strlen(buf)+1,sizeof(char));
		if(b->ufnm==NULL) err(a->oerr,113,"no space");
		strcpy(b->ufnm,buf);
		b->uend=0;
		if(isdev(buf))
		{	b->ufd = fopen(buf,"r");
			if(b->ufd==NULL) err(a->oerr,errno,buf)
			else	b->uwrt = 0;
		}
		else
		{	b->ufd = fopen(buf, "a");
			if(b->ufd != NULL) b->uwrt = 1;
			else if((b->ufd = fopen(buf, "r")) != NULL)
			{	fseek(b->ufd, 0L, 2);
				b->uwrt = 0;
			}
			else	err(a->oerr, errno, buf)
		}
		b->useek=canseek(b->ufd);
		if((b->uinode=inode(buf))==-1)
			err(a->oerr,108,"open")
		if(a->orl && b->useek) rewind(b->ufd);
		return(0);
	 case 's':
		b->uscrtch=1;
		strcpy(buf,"tmp.FXXXXXX");
		close(mkstemp(buf));
		goto done;
	case 'n':
		b->uscrtch=0;
		if(a->ofnm==0) err(a->oerr,107,"open")
		g_char(a->ofnm,a->ofnmlen,buf);
		/*SYSDEP access*/
		if(access(buf, 0) == 0) creat(buf, 0666);
		goto done;
	}
connected:
	if(a->ofnm==0)
	{
	same:	if(a->oblnk!= 0) b->ublnk= *a->oblnk== 'b'?0:1;
		return(0);
	}
	g_char(a->ofnm,a->ofnmlen,buf);
	if(inode(buf)==b->uinode) goto same;
	x.cunit=a->ounit;
	x.csta=0;
	x.cerr=a->oerr;
	if((n=f_clos(&x))!=0) return(n);
	goto unconnected;
}

int
fk_open(int rd,int seq,int fmt, ftnint n)
{
	char nbuf[10];
	olist a;
	sprintf(nbuf,"fort.%ld",n);
	a.oerr=1;
	a.ounit=n;
	a.ofnm=nbuf;
	a.ofnmlen=strlen(nbuf);
	a.osta=NULL;
	a.oacc= seq==SEQ?"s":"d";
	a.ofm = fmt==FMT?"f":"u";
	a.orl = seq==DIR?1:0;
	a.oblnk=NULL;
	return(f_open(&a));
}

int
isdev(char *s)
{	struct stat x;
	int j;
	if(stat(s, &x) == -1) return(0);
	if((j = (x.st_mode&S_IFMT)) == S_IFREG || j == S_IFDIR) return(0);
	else	return(1);
}

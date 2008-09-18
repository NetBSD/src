/*	$Id: endfile.c,v 1.1.1.1.2.2 2008/09/18 05:15:46 wrstuden Exp $	*/
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
#include <sys/wait.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fio.h"
static alist *ax;

int
f_end(alist *a)
{
	unit *b;
	if(a->aunit>=MXUNIT || a->aunit<0) err(a->aerr,101,"endfile");
	b = &units[a->aunit];
	if(b->ufd==NULL) return(0);
	b->uend=1;
	if( b->useek==0) return(0);
	ax=a;
	if(b->uwrt) nowreading(b);
	return(t_runc(b));
}

int
t_runc(unit *b)
{
	char buf[128],nm[16];
	FILE *tmp;
	int n,m, fd;
	long loc,len;

	if(b->url) return(0);	/*don't truncate direct files*/
	loc=ftell(b->ufd);
	fseek(b->ufd,0L,2);
	len=ftell(b->ufd);
	if(loc==len || b->useek==0 || b->ufnm==NULL) return(0);
	strcpy(nm,"tmp.FXXXXXX");
	if(b->uwrt) nowreading(b);
	fd = mkstemp(nm);
	tmp=fdopen(fd,"w");
	fseek(b->ufd,0L,0);
	for(;loc>0;)
	{
		n=fread(buf,1,loc>128?128:(int)loc,b->ufd);
		if(n>loc) n=loc;
		loc -= n;
		fwrite(buf,1,n,tmp);
	}
	fflush(tmp);
	for(n=0;n<10;n++)
	{
		if((m=fork())==-1) continue;
		else if(m==0)
		{
			execl("/bin/cp","cp",nm,b->ufnm,NULL);
			execl("/usr/bin/cp","cp",nm,b->ufnm,NULL);
			fprintf(stdout,"no cp\n");
			exit(1);
		}
		wait(&m);
		if(m!=0) err(ax->aerr,111,"endfile");
		fclose(tmp);
		unlink(nm);
		return(0);
	}
	err(ax->aerr,111,"endfile");
}

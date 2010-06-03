/*	Id: close.c,v 1.3 2008/03/01 13:44:12 ragge Exp 	*/	
/*	$NetBSD: close.c,v 1.1.1.2 2010/06/03 18:58:10 plunky Exp $	*/
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
#include <stdlib.h>
#include <unistd.h>

#include "fio.h"

int
f_clos(cllist *a)
{	unit *b;
	if(a->cunit >= MXUNIT) return(0);
	b= &units[a->cunit];
	if(b->ufd==NULL) return(0);
	b->uend=0;
	if(a->csta!=0)
		switch(*a->csta)
		{
		default:
		keep:
		case 'k':
			if(b->uwrt) t_runc(b);
			fclose(b->ufd);
			if(b->ufnm!=0) free(b->ufnm);
			b->ufnm=NULL;
			b->ufd=NULL;
			return(0);
		case 'd':
		delete:
			fclose(b->ufd);
			if(b->ufnm!=0)
			{	unlink(b->ufnm); /*SYSDEP*/
				free(b->ufnm);
			}
			b->ufnm=NULL;
			b->ufd=NULL;
			return(0);
		}
	else if(b->uscrtch==1) goto delete;
	else goto keep;
}

void
f_exit()
{	int i;
	cllist xx;
	xx.cerr=1;
	xx.csta=NULL;
	for(i=0;i<MXUNIT;i++)
	{
		xx.cunit=i;
		f_clos(&xx);
	}
}

void
flush_()
{	int i;
	for(i=0;i<MXUNIT;i++)
		if(units[i].ufd != NULL) fflush(units[i].ufd);
}

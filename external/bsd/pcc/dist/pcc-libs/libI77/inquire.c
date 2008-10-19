/*	$Id: inquire.c,v 1.1.1.1.6.2 2008/10/19 22:40:27 haad Exp $	*/
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
f_inqu(inlist *a)
{
	flag byfile,legal;
	int i;
	unit *p;
	char buf[256];
	long x;

	x = 0; /* XXX - check correctness */
	if(a->infile!=NULL) {
		byfile=1;
		g_char(a->infile,a->infilen,buf);
		x=inode(buf);
		for(i=0,p=NULL;i<MXUNIT;i++)
			if(units[i].uinode==x && units[i].ufd!=NULL)
				p = &units[i];
	} else {
		byfile=0;
		if(a->inunit<MXUNIT && a->inunit>=0) {
			legal=1;
			p= &units[a->inunit];
		} else {
			legal=0;
			p=NULL;
		}
	}
	if(a->inex!=NULL) {
		if((byfile && x>0) || (!byfile && p!=NULL))
			*a->inex=1;
		else *a->inex=0;
	}
	if(a->inopen!=NULL) {
		if(byfile) *a->inopen=(p!=NULL);
		else *a->inopen=(p!=NULL && p->ufd!=NULL);
	}
	if(a->innum!=NULL) *a->innum= p-units;
	if(a->innamed!=NULL) {
		if(byfile || (p!=NULL && p->ufnm!=NULL))
			*a->innamed=1;
		else	*a->innamed=0;
	}
	if(a->inname!=NULL) {
		if(byfile)
			b_char(buf,a->inname,a->innamlen);
		else if(p!=NULL && p->ufnm!=NULL)
			b_char(p->ufnm,a->inname,a->innamlen);
	}
	if(a->inacc!=NULL && p!=NULL && p->ufd!=NULL) {
		if(p->url)
			b_char("direct",a->inacc,a->inacclen);
		else	b_char("sequential",a->inacc,a->inacclen);
	}
	if(a->inseq!=NULL) {
		if(byfile || (p!=NULL && p->useek))
			b_char("yes",a->inseq,a->inseqlen);
		else	b_char("no",a->inseq,a->inseqlen);
	}
	if(a->indir!=NULL) {
		if(byfile || (p!=NULL && p->useek))
			b_char("yes",a->indir,a->indirlen);
		else	b_char("no",a->indir,a->indirlen);
	}
	if(a->infmt!=NULL) {
		if(p!=NULL && p->ufmt)
			b_char("formatted",a->infmt,a->infmtlen);
		else if(p!=NULL)
			b_char("unformatted",a->infmt,a->infmtlen);
	}
	if(a->inform!=NULL)
		b_char("yes",a->inform,a->informlen);
	if(a->inunf) {
		if(byfile || (p!=NULL && p->useek))
			b_char("yes",a->inunf,a->inunflen);
		else	b_char("unknown",a->inunf,a->inunflen);
	}
	if(a->inrecl!=NULL && p!=NULL)
		*a->inrecl=p->url;
	if(a->innrec!=NULL && p!=NULL && p->url>0)
		*a->innrec=ftell(p->ufd)/p->url+1;
	if(a->inblank && p!=NULL && p->ufmt) {
		if(p->ublnk)
			b_char("zero",a->inblank,a->inblanklen);
		else	b_char("blank",a->inblank,a->inblanklen);
	}
	return(0);
}

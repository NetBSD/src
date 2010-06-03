/*	Id: pow_zi.c,v 1.3 2008/02/27 17:09:54 ragge Exp 	*/	
/*	$NetBSD: pow_zi.c,v 1.1.1.2 2010/06/03 18:58:08 plunky Exp $	*/
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
#include "f77lib.h"

void
pow_zi(dcomplex *p, dcomplex *a, long int *b) 	/* p = a**b  */
{
	long int n;
	double t;
	dcomplex x;

	n = *b;
	p->dreal = 1;
	p->dimag = 0;

	if(n == 0)
		return;
	if(n < 0) {
		n = -n;
		z_div(&x, p, a);
	} else {
		x.dreal = a->dreal;
		x.dimag = a->dimag;
	}

	for( ; ; ) {
		if(n & 01) {
			t = p->dreal * x.dreal - p->dimag * x.dimag;
			p->dimag = p->dreal * x.dimag + p->dimag * x.dreal;
			p->dreal = t;
		}
		if(n >>= 1) {
			t = x.dreal * x.dreal - x.dimag * x.dimag;
			x.dimag = 2 * x.dreal * x.dimag;
			x.dreal = t;
		} else
			break;
	}
}

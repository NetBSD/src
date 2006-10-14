/*	$NetBSD: print.c,v 1.1 2006/10/14 21:08:50 christos Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: print.c,v 1.1 2006/10/14 21:08:50 christos Exp $");

#include <stdio.h>

#include "lint1.h"
#include "externs1.h"

#ifdef DEBUG
static const char *str_op_t[] =
{
	"*noop*",
	"->",
	".",
	"!",
	"~",
	"++",
	"--",
	"++<",
	"--<",
	"++>",
	"-->",
	"+",
	"-",
	"*",
	"&",
	"*",
	"/",
	"%",
	"+",
	"-",
	"<<",
	">>",
	"<",
	"<=",
	">",
	">=",
	"==",
	"!=",
	"&",
	"^",
	"|",
	"&&",
	"||",
	"?",
	":",
	"=",
	"*=",
	"/=",
	"%=",
	"+=",
	"-=",
	"<<=",
	">>=",
	"&=",
	"^=",
	"|=",
	"*name*",
	"*constant*",
	"*string*",
	"*field select*",
	"*call*",
	",",
	"*(cast)*",
	"*icall*",
	"*load*",
	"*push*",
	"return",
	"*init*",
	"*case*",
	"*farg*",
};

char *
prtnode(char *buf, size_t bufsiz, const tnode_t *tn)
{
	strg_t *st;
	val_t *v;
	sym_t *s;
	switch (tn->tn_op) {
	case NAME:
		s = tn->tn_sym;
		(void)snprintf(buf, bufsiz, "%s", s->s_name);
		break;
	case CON:
		v = tn->tn_val;
		switch (v->v_tspec) {
		case FLOAT:
		case DOUBLE:
		case LDOUBLE:
			(void)snprintf(buf, bufsiz, "%Lg", v->v_ldbl);
			break;
		default:
			(void)snprintf(buf, bufsiz, v->v_ansiu ? "%llu" :
			    "%lld", v->v_quad);
			break;
		}
		break;
		
	case STRING:
		st = tn->tn_strg;
		switch (st->st_tspec) {
		case CHAR:
		case SCHAR:
		case UCHAR:
			(void)snprintf(buf, bufsiz, "\"%s\"", st->st_cp);
			break;
		default:
			(void)snprintf(buf, bufsiz, "\"*wide string*\"");
			break;
		}
		break;
	default:
		(void)snprintf(buf, bufsiz, "%s", str_op_t[tn->tn_op]);
		break;
	}
	return buf;
}
#endif

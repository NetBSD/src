/*	$NetBSD: expr.c,v 1.19.46.1 2019/06/10 22:10:21 christos Exp $	*/
/* $OpenBSD: expr.c,v 1.17 2006/01/20 23:10:19 espie Exp $ */
/*
 * Copyright (c) 2004 Marc Espie <espie@cvs.openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif
#include <sys/cdefs.h>
__RCSID("$NetBSD: expr.c,v 1.19.46.1 2019/06/10 22:10:21 christos Exp $");
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include "mdef.h"
#include "extern.h"

int32_t end_result;
const char *copy_toeval;

extern void yy_scan_string(const char *);
extern int yyparse(void);
extern int yyerror(const char *);

int
yyerror(const char *msg)
{
	fprintf(stderr, "m4:%s:%lu: %s in expr %s\n", infile[ilevel].name,
	    infile[ilevel].lineno, msg, copy_toeval);
	return(0);
}

int 
expr(const char *toeval)
{
	copy_toeval = toeval;
	yy_scan_string(toeval);
	yyparse();
	return end_result;
}

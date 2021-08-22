#	$NetBSD: Makefile.inc,v 1.11 2021/08/22 22:24:11 rillig Exp $
#	from: @(#)Makefile.inc	8.1 (Berkeley) 6/6/93

WARNS?=	5
BINDIR?=/usr/bin

.include <bsd.own.mk>
.if ${MKLINT} != "no" && ${LINT_SUPPORTED:Uyes} == "yes"
realall: lint
.endif

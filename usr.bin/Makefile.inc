#	$NetBSD: Makefile.inc,v 1.12 2024/04/20 13:24:49 rillig Exp $
#	from: @(#)Makefile.inc	8.1 (Berkeley) 6/6/93

WARNS?=	5
BINDIR?=/usr/bin

.include <bsd.own.mk>
.if ${MKLINT} != "no" && !defined(NOLINT)
realall: lint
.endif

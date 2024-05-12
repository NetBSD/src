#	$NetBSD: Makefile.inc,v 1.13 2024/05/12 17:55:37 rillig Exp $
#	from: @(#)Makefile.inc	8.1 (Berkeley) 6/6/93

WARNS?=	5
BINDIR?=/usr/bin

LINTFLAGS+=	-u	# don't warn about "used but not defined"

.include <bsd.own.mk>
.if ${MKLINT} != "no" && !defined(NOLINT)
realall: lint
.endif

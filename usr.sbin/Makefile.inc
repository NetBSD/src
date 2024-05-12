#	$NetBSD: Makefile.inc,v 1.10 2024/05/12 17:55:37 rillig Exp $
#	from: @(#)Makefile.inc	8.1 (Berkeley) 6/6/93

BINDIR?=	/usr/sbin
WARNS?=		4
CPPFLAGS+=	-D_KERNTYPES

LINTFLAGS+=	-u	# don't warn about "used but not defined"

.include <bsd.own.mk>
.if ${MKLINT} != "no" && !defined(NOLINT)
realall: lint
.endif

#	$NetBSD: Makefile.inc,v 1.9 2024/04/20 13:24:49 rillig Exp $
#	from: @(#)Makefile.inc	8.1 (Berkeley) 6/6/93

BINDIR?=	/usr/sbin
WARNS?=		4
CPPFLAGS+=	-D_KERNTYPES

.include <bsd.own.mk>
.if ${MKLINT} != "no" && !defined(NOLINT)
realall: lint
.endif

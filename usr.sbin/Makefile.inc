#	$NetBSD: Makefile.inc,v 1.8 2021/09/14 20:36:02 rillig Exp $
#	from: @(#)Makefile.inc	8.1 (Berkeley) 6/6/93

BINDIR?=	/usr/sbin
WARNS?=		4
CPPFLAGS+=	-D_KERNTYPES

.include <bsd.own.mk>
.if ${MKLINT} != "no" && ${LINT_SUPPORTED:Uyes} == "yes"
realall: lint
.endif

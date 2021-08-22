#	$NetBSD: Makefile,v 1.21 2021/08/22 14:35:44 rillig Exp $

.PATH:		${.CURDIR}/../../mkdep

PROG=		xlint
SRCS=		xlint.c mem.c findcc.c
PROGNAME= 	lint
MAN=		lint.1

CPPFLAGS+=	-I${.CURDIR}/../lint1
CPPFLAGS+=	-I${.CURDIR}/../../mkdep

.if (${HOSTPROG:U} == "")
DPADD+=		${LIBUTIL}
LDADD+=		-lutil

FILES+=		strict-bool-stdbool.h
FILESDIR=	/usr/libdata/lint/strict-bool
FILESNAME_strict-bool-stdbool.h= stdbool.h
.endif

.include <bsd.prog.mk>

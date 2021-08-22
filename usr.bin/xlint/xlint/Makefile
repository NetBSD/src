#	$NetBSD: Makefile,v 1.22 2021/08/22 15:06:49 rillig Exp $

.PATH:		${.CURDIR}/../../mkdep

PROG=		xlint
SRCS=		xlint.c mem.c findcc.c
PROGNAME= 	lint
MAN=		lint.1

CPPFLAGS+=	-I${.CURDIR}/../lint1
CPPFLAGS+=	-I${.CURDIR}/../../mkdep
CPPFLAGS+=	-DIS_XLINT

.if (${HOSTPROG:U} == "")
DPADD+=		${LIBUTIL}
LDADD+=		-lutil

FILES+=		strict-bool-stdbool.h
FILESDIR=	/usr/libdata/lint/strict-bool
FILESNAME_strict-bool-stdbool.h= stdbool.h
.endif

.include <bsd.prog.mk>

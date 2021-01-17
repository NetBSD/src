#	$NetBSD: Makefile,v 1.19 2021/01/17 19:32:53 rillig Exp $

.PATH:		${.CURDIR}/../lint1
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
.endif

FILES+=		strict-bool-stdbool.h
FILESDIR=	/usr/libdata/lint/strict-bool
FILESNAME_strict-bool-stdbool.h= stdbool.h

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.18 2021/01/16 22:18:14 rillig Exp $

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

#FILES+=		strict-bool-stdbool.h
#FILESDIR=	/usr/libdata/lint/strict-bool
#FILESNAME_strict-bool-stdbool.h= stdbool.h

.include <bsd.prog.mk>

#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.11.2.1 1997/11/08 15:12:22 lukem Exp $

PROG=	pwd_mkdb
SRCS=	pwd_mkdb.c getpwent.c
MAN=	pwd_mkdb.8
LDADD+=	-lutil
DPADD+=	${LIBUTIL}
CPPFLAGS+=	-I${.CURDIR}/../../lib/libc/include

.PATH:	${.CURDIR}/../../lib/libc/gen

.include <bsd.prog.mk>

getpwent.o: getpwent.c
	${COMPILE.c} -UYP ${.IMPSRC}

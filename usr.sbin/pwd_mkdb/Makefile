#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.13 2000/01/23 19:59:32 mycroft Exp $

PROG=	pwd_mkdb
SRCS=	pwd_mkdb.c getpwent.c
MAN=	pwd_mkdb.8
LDADD+=	-lutil
DPADD+=	${LIBUTIL}
CPPFLAGS+=	-I${.CURDIR}/../../lib/libc/include -D__DBINTERFACE_PRIVATE

.PATH:	${.CURDIR}/../../lib/libc/gen

.include <bsd.prog.mk>

getpwent.o: getpwent.c
	${COMPILE.c} -UYP ${.IMPSRC}

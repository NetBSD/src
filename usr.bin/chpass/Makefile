#	$NetBSD: Makefile,v 1.11 2002/03/22 18:10:22 thorpej Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/2/94

.include <bsd.own.mk>

PROG=	chpass
SRCS=	chpass.c edit.c field.c table.c util.c getpwent.c
BINOWN=	root
BINMODE=4555
.PATH:	${.CURDIR}/../../lib/libc/gen
LINKS=	${BINDIR}/chpass ${BINDIR}/chfn ${BINDIR}/chpass ${BINDIR}/chsh
MLINKS=	chpass.1 chfn.1 chpass.1 chsh.1

CPPFLAGS+=-I${.CURDIR}/../../lib/libc/include

.if (${USE_YP} != "no")
SRCS+=	pw_yp.c
CPPFLAGS+=-DYP
DPADD+=	${LIBRPCSVC}
LDADD+=	-lrpcsvc
.endif

DPADD+= ${LIBUTIL}
LDADD+= -lutil

.include <bsd.prog.mk>

getpwent.o: getpwent.c
	${COMPILE.c} -UYP ${.IMPSRC}

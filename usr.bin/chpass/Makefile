#	$NetBSD: Makefile,v 1.14 2003/07/22 12:25:57 lukem Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/2/94

.include <bsd.own.mk>

PROG=	chpass
SRCS=	chpass.c edit.c field.c table.c util.c
BINOWN=	root
BINMODE=4555
.PATH:	${NETBSDSRCDIR}/lib/libc/gen
LINKS=	${BINDIR}/chpass ${BINDIR}/chfn ${BINDIR}/chpass ${BINDIR}/chsh
MLINKS=	chpass.1 chfn.1 chpass.1 chsh.1

CPPFLAGS+=-I${NETBSDSRCDIR}/lib/libc/include

.if (${USE_YP} != "no")
SRCS+=	pw_yp.c
CPPFLAGS+=-DYP
DPADD+=	${LIBRPCSVC}
LDADD+=	-lrpcsvc
.else
SRCS+=	getpwent.c
CPPFLAGS.getpwent.c=-UYP
.endif

DPADD+= ${LIBUTIL}
LDADD+= -lutil

.include <bsd.prog.mk>

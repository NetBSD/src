#	$NetBSD: Makefile,v 1.10 1997/10/24 09:00:22 lukem Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/2/94

PROG=	chpass
SRCS=	chpass.c edit.c field.c pw_yp.c table.c util.c getpwent.c
BINOWN=	root
BINMODE=4555
.PATH:	${.CURDIR}/../../lib/libc/gen
LINKS=	${BINDIR}/chpass ${BINDIR}/chfn ${BINDIR}/chpass ${BINDIR}/chsh
MLINKS=	chpass.1 chfn.1 chpass.1 chsh.1
CPPFLAGS+=-DYP -I${.CURDIR}/../../lib/libc/include
DPADD+= ${LIBRPCSVC} ${LIBUTIL}
LDADD+= -lrpcsvc -lutil

.include <bsd.prog.mk>

getpwent.o: getpwent.c
	${COMPILE.c} -UYP ${.IMPSRC}

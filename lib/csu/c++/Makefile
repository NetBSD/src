#	$NetBSD: Makefile,v 1.17 2008/07/21 15:34:58 lukem Exp $

.include <bsd.own.mk>

CPPFLAGS+=	-DLIBC_SCCS
.if ${MKPIC} != "no"
COPTS+=		-fPIC
.endif
OBJS=		c++rt0.o

realall: ${OBJS}

c++rt0.o: c++rt0.c
	${_MKTARGET_COMPILE}
	${COMPILE.c} ${.ALLSRC}
	${LD} -x -r ${.TARGET}
	mv a.out ${.TARGET}

FILES=${OBJS}
FILESDIR=${LIBDIR}

.include <bsd.prog.mk>

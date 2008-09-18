#	$NetBSD: Makefile,v 1.16.32.1 2008/09/18 04:39:19 wrstuden Exp $

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

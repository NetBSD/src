#	$NetBSD: Makefile,v 1.15 2004/02/14 08:41:58 simonb Exp $

CPPFLAGS+=	-DLIBC_SCCS
.if ${MKPIC} != "no"
COPTS+=		-fPIC
.endif
OBJS=		c++rt0.o

realall: ${OBJS}

c++rt0.o: c++rt0.c
	${COMPILE.c} ${.ALLSRC}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}

FILES=${OBJS}
FILESDIR=${LIBDIR}

.include <bsd.prog.mk>

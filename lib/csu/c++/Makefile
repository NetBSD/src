#	$NetBSD: Makefile,v 1.14 2000/04/29 17:42:46 dmcmahill Exp $

CPPFLAGS+=	-DLIBC_SCCS
COPTS+=		-fPIC
OBJS=		c++rt0.o

realall: ${OBJS}

c++rt0.o: c++rt0.c
	${COMPILE.c} ${.ALLSRC}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}

FILES=${OBJS}
FILESDIR=${LIBDIR}

.include <bsd.prog.mk>

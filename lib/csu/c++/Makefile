#	$NetBSD: Makefile,v 1.13 2000/01/22 19:39:20 mycroft Exp $

CPPFLAGS+=	-DLIBC_SCCS
COPTS+=		-fpic
OBJS=		c++rt0.o

realall: ${OBJS}

c++rt0.o: c++rt0.c
	${COMPILE.c} ${.ALLSRC}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}

FILES=${OBJS}
FILESDIR=${LIBDIR}

.include <bsd.prog.mk>

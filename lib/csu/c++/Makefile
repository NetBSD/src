#	$NetBSD: Makefile,v 1.12 1999/04/03 16:28:50 simonb Exp $

CPPFLAGS+=	-DLIBC_SCCS
COPTS+=		-fpic
OBJS=		c++rt0.o

all: ${OBJS}

c++rt0.o: c++rt0.c
	${COMPILE.c} ${.ALLSRC}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}

FILES=${OBJS}
FILESDIR=${LIBDIR}

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.11 1999/02/27 03:27:54 tv Exp $

CPPFLAGS+=	-DLIBC_SCCS
COPTS+=		-fpic
OBJS=		c++rt0.o

all: ${OBJS}

c++rt0.o: c++rt0.c
	${COMPILE.c} ${.ALLSRC}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}

FILES=${OBJS}
FILESDIR=/usr/lib

.include <bsd.prog.mk>

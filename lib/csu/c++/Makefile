#	$NetBSD: Makefile,v 1.11.2.1 2000/08/26 18:32:12 he Exp $

CPPFLAGS+=	-DLIBC_SCCS
COPTS+=		-fPIC
OBJS=		c++rt0.o

all: ${OBJS}

c++rt0.o: c++rt0.c
	${COMPILE.c} ${.ALLSRC}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}

FILES=${OBJS}
FILESDIR=/usr/lib

.include <bsd.prog.mk>

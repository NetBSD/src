#	$NetBSD: Makefile,v 1.7 1997/03/24 22:11:48 christos Exp $

CFLAGS+=	-DLIBC_SCCS
.if (${MACHINE_ARCH} != "powerpc")
CFLAGS+=	-fpic
.endif
OBJS=		c++rt0.o

all: ${OBJS}

c++rt0.o: c++rt0.c
	${COMPILE.c} ${.ALLSRC}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}

FILES=${OBJS}
FILESDIR=/usr/lib

.include <bsd.prog.mk>

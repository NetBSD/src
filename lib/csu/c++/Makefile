#	$NetBSD: Makefile,v 1.9 1997/05/31 21:21:41 cjs Exp $

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

#	$NetBSD: Makefile,v 1.9.2.1 1997/11/09 00:59:20 lukem Exp $

CPPFLAGS+=	-DLIBC_SCCS
.if (${MACHINE_ARCH} != "powerpc")
COPTS+=		-fpic
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

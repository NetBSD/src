#	$NetBSD: Makefile,v 1.10 1997/10/22 16:01:28 lukem Exp $

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

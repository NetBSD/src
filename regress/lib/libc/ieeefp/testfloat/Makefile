# $NetBSD: Makefile,v 1.2 2001/03/22 12:27:59 ross Exp $

TOSRC=		${.CURDIR}/../../../../..
SFLIB=		${TOSRC}/lib/libc/softfloat
.PATH:		${SFLIB} ${.CURDIR}/arch/${MACHINE_ARCH}

MKMAN=		no
WARNS=		2
CPPFLAGS+=	-I${.CURDIR}/include -I${SFLIB}
LDADD+=		-lm

PROG=		testfloat
SRCS+=		testfloat.c softfloat.c fail.c testCases.c writeHex.c	\
			testLoops.c testFunction.c csystfloat.c

regress: $(PROG)
	./${PROG} -all

.-include "${.CURDIR}/arch/${MACHINE_ARCH}/Makefile.inc"
.include <bsd.prog.mk>

# $NetBSD: Makefile,v 1.3 2001/12/12 01:24:02 tv Exp $

TOSRC=		${.CURDIR}/../../../../..
SFLIB=		${TOSRC}/lib/libc/softfloat
.PATH:		${SFLIB} ${.CURDIR}/arch/${MACHINE_ARCH}

NOMAN=		# defined
WARNS=		2
CPPFLAGS+=	-I${.CURDIR}/include -I${SFLIB}
LDADD+=		-lm

PROG=		testfloat
SRCS+=		testfloat.c softfloat.c fail.c testCases.c writeHex.c \
		testLoops.c testFunction.c csystfloat.c

regress: $(PROG)
	./${PROG} -all

.-include "${.CURDIR}/arch/${MACHINE_ARCH}/Makefile.inc"
.include <bsd.prog.mk>

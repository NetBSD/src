# $NetBSD: Makefile,v 1.4 2002/05/21 23:58:42 bjh21 Exp $

TOSRC=		${.CURDIR}/../../../../..
SOFTFLOAT_BITS=	64
SFLIB1=		${TOSRC}/lib/libc/softfloat
SFLIB2=		${TOSRC}/lib/libc/softfloat/bits${SOFTFLOAT_BITS}
.PATH:		${SFLIB1} ${SFLIB2} ${.CURDIR}/arch/${MACHINE_ARCH}

NOMAN=		# defined
WARNS=		2
CPPFLAGS+=	-I${.CURDIR}/include -I${SFLIB1} -I${SFLIB2}
LDADD+=		-lm

PROG=		testfloat
SRCS+=		testfloat.c softfloat.c fail.c testCases.c writeHex.c \
		testLoops.c testFunction.c csystfloat.c

regress: $(PROG)
	./${PROG} -all

.-include "${.CURDIR}/arch/${MACHINE_ARCH}/Makefile.inc"
.include <bsd.prog.mk>

# $NetBSD: Makefile,v 1.5 2002/09/18 04:16:00 lukem Exp $

NOMAN=		# defined

.include <bsd.own.mk>

SOFTFLOAT_BITS=	64
SFLIB1=		${NETBSDSRCDIR}/lib/libc/softfloat
SFLIB2=		${NETBSDSRCDIR}/lib/libc/softfloat/bits${SOFTFLOAT_BITS}
.PATH:		${SFLIB1} ${SFLIB2} ${.CURDIR}/arch/${MACHINE_ARCH}

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

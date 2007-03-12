#	$NetBSD: Makefile,v 1.2 2007/03/12 03:39:57 enami Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	hntest
SRCS=	hntest.c

# Pass -DINTREE to make to test the one in source tree directly.
.if defined(INTREE)
.PATH:  	${NETBSDSRCDIR}/lib/libc/gen
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/include
SRCS+=	humanize_number.c
.endif

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>

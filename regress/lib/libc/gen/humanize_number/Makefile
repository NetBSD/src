#	$NetBSD: Makefile,v 1.1 2004/07/14 22:47:31 enami Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	hntest
SRCS=	hntest.c

# Pass -DINTREE to make to test the one in source tree directly.
.if defined(INTREE)
.PATH:  ${NETBSDSRCDIR}/lib/libc/gen
SRCS+=	humanize_number.c
.endif

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>

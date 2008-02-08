#	$NetBSD: Makefile,v 1.1 2008/02/08 20:04:50 christos Exp $

NOMAN=		# defined

PROG=		thread
WARNS?=		4
CFLAGS+=-pthread
DPADD+=${LIBOBJC}
LDADD+=-lobjc

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>

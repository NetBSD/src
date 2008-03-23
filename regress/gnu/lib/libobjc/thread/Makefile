#	$NetBSD: Makefile,v 1.2.4.2 2008/03/23 00:42:58 matt Exp $

NOMAN=		# defined

PROG=		thread
WARNS?=		4
CFLAGS+=-pthread
LDFLAGS+=-pthread
DPADD+=${LIBOBJC}
LDADD+=-lobjc

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>

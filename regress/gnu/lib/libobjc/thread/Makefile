#	$NetBSD: Makefile,v 1.2 2008/02/09 08:38:08 mrg Exp $

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

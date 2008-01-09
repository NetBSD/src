#	$NetBSD: Makefile,v 1.1.4.2 2008/01/09 01:37:38 matt Exp $

NOMAN=		# defined

PROG=		softint1

LDADD+=		-lpthread
DPADD+=		${LIBPTHREAD}

regress:
	@./${PROG}

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.1 2007/12/04 08:02:24 ad Exp $

NOMAN=		# defined

PROG=		softint1

LDADD+=		-lpthread
DPADD+=		${LIBPTHREAD}

regress:
	@./${PROG}

.include <bsd.prog.mk>

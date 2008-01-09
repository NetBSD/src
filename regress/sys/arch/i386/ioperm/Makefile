#	$NetBSD: Makefile,v 1.1.2.2 2008/01/09 01:37:31 matt Exp $

NOMAN=		# defined

PROG=		ioperm
WARNS=		4
DPADD+=		${LIBARCH}
LDADD+=		-li386

regress:
	-@if ./${PROG}; \
	then echo PASSED; \
	else echo FAILED; fi

.include <bsd.prog.mk>

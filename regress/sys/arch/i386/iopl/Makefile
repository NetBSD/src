#	$NetBSD: Makefile,v 1.1.2.2 2008/01/09 01:37:32 matt Exp $

NOMAN=		# defined

PROG=		iopl
WARNS=		4
DPADD+=		${LIBARCH}
LDADD+=		-li386

regress:
	-@if ./${PROG}; \
	then echo PASSED; \
	else echo FAILED; fi

.include <bsd.prog.mk>

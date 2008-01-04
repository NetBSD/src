#	$NetBSD: Makefile,v 1.1 2008/01/04 14:23:00 yamt Exp $

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

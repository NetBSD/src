#	$NetBSD: Makefile,v 1.8 2001/12/12 01:24:13 tv Exp $

PROG=		testldt
NOMAN=		# defined
WARNS=		2
DPADD+=		${LIBARCH}
LDADD+=		-li386

regress:
	-@if ./testldt; \
	then echo PASSED; \
	else echo FAILED; fi

.include <bsd.prog.mk>

# C Optimizer (-O) breaks this program - don't use !!
DBG:=		-O0

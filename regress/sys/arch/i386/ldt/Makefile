#	$NetBSD: Makefile,v 1.9 2002/09/18 05:41:50 lukem Exp $

NOMAN=		# defined

PROG=		testldt
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

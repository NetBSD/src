#	$NetBSD: Makefile,v 1.5 1998/04/13 14:28:04 lukem Exp $
#
PROG=	testldt
NOMAN=	noman

DPADD+=	${LIBARCH}
LDADD+=	-li386

regress:
	-@if ./testldt; \
	then echo PASSED; \
	else echo FAILED; fi

.include <bsd.prog.mk>

# C Optimizer (-O) breaks this program - don't use !!
COPTS+=-O0

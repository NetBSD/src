#	$NetBSD: Makefile,v 1.6 1999/02/13 02:54:32 lukem Exp $
#
PROG=	testldt
MKMAN=	no

DPADD+=	${LIBARCH}
LDADD+=	-li386

regress:
	-@if ./testldt; \
	then echo PASSED; \
	else echo FAILED; fi

.include <bsd.prog.mk>

# C Optimizer (-O) breaks this program - don't use !!
COPTS+=-O0

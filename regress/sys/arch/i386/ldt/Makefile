#	$NetBSD: Makefile,v 1.7 2001/06/10 16:32:05 sommerfeld Exp $
#
PROG=	testldt
MKMAN=	no
WARNS=2

DPADD+=	${LIBARCH}
LDADD+=	-li386

regress:
	-@if ./testldt; \
	then echo PASSED; \
	else echo FAILED; fi

.include <bsd.prog.mk>

# C Optimizer (-O) breaks this program - don't use !!
COPTS+=-O0

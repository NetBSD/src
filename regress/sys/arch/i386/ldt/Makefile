#	$NetBSD: Makefile,v 1.4 1997/05/07 16:01:46 mycroft Exp $
#
# C Optimizer (-O) breaks this program - don't use !!
CFLAGS=-g
PROG=	testldt
NOMAN=	noman

DPADD+=	${LIBARCH}
LDADD+=	-li386

regress:
	-@if ./testldt; \
	then echo PASSED; \
	else echo FAILED; fi

.include <bsd.prog.mk>

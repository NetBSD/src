#	$Id: Makefile,v 1.2 1994/01/16 15:50:19 brezak Exp $
#
# C Optimizer (-O) breaks this program - don't use !!
CFLAGS=-g
PROG=	testldt
NOMAN=	noman

DPADD+=	${LIBARCH}
LDADD+=	-li386

install:

regress:
	-@if ./testldt; \
	then echo PASSED; \
	else echo FAILED; fi


.include <bsd.prog.mk>


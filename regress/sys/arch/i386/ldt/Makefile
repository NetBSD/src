CFLAGS+=-g
PROG=	testldt
NOMAN=	noman

DPADD+=	${LIBARCH}
LDADD+=	-li386

install:

regress:
	-if ./testldt; \
	then echo PASSED; \
	else echo FAILED; fi


.include <bsd.prog.mk>


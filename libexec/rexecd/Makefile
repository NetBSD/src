#	@(#)Makefile	5.3 (Berkeley) 5/11/90

PROG=	rexecd
MAN8=	rexecd.0
.ifndef EXPORTABLE_SYSTEM
DPADD+= ${LIBCRYPT}
LDADD+= -lcrypt
.endif

.include <bsd.prog.mk>

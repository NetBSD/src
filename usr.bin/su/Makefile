#	@(#)Makefile	5.5 (Berkeley) 5/11/90

PROG=	su
BINOWN=	root
BINMODE=4555
.ifndef EXPORTABLE_SYSTEM
DPADD+= ${LIBCRYPT}
LDADD+= -lcrypt
.endif

.include <bsd.prog.mk>

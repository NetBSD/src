#	@(#)Makefile	5.5 (Berkeley) 4/16/91

PROG=	lock
BINOWN= root
BINMODE=4555
.ifndef EXPORTABLE_SYSTEM
DPADD+= ${LIBCRYPT}
LDADD+= -lcrypt
.endif

.include <bsd.prog.mk>

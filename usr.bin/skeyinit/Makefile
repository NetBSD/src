# $Id: Makefile,v 1.2 1994/12/22 12:30:58 cgd Exp $

PROG= skeyinit
BINOWN=root
BINMODE=4555
CFLAGS+= -I${.CURDIR}/../../lib/libskey
DPADD=	${LIBCRYPT} ${LIBSKEY}
LDADD=	-lcrypt -lskey

.include <bsd.prog.mk>

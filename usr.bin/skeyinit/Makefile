# $Id: Makefile,v 1.1 1994/05/24 06:48:08 deraadt Exp $

PROG= skeyinit
BINOWN=root
BINMODE=4555
CFLAGS+= -I${.CURDIR}/../../lib/libskey
MAN1= skeyinit.0
DPADD=	${LIBCRYPT} ${LIBSKEY}
LDADD=	-lcrypt -lskey

.include <bsd.prog.mk>

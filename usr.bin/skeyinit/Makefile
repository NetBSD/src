#	$NetBSD: Makefile,v 1.3 1997/01/09 20:21:39 tls Exp $

PROG= skeyinit
BINOWN=root
BINMODE=4555
CFLAGS+= -I${.CURDIR}/../../lib/libskey
DPADD=	${LIBCRYPT} ${LIBSKEY}
LDADD=	-lcrypt -lskey

.include <bsd.prog.mk>

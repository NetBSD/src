# $Id: Makefile,v 1.1 1994/05/24 06:47:42 deraadt Exp $

PROG= skey
CFLAGS+= -I${.CURDIR}/../../lib/libskey
MAN1= skey.0
DPADD=	${LIBSKEY}
LDADD=	-lskey

beforeinstall:
	install -c -m 755 ${.CURDIR}/keyaudit ${BINDIR}
	install -c -m 755 ${.CURDIR}/keyinfo ${BINDIR}

.include <bsd.prog.mk>

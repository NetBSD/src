# $Id: Makefile,v 1.2 1994/05/31 08:46:10 deraadt Exp $

PROG= skey
CFLAGS+= -I${.CURDIR}/../../lib/libskey
MAN1= skey.0
DPADD=	${LIBSKEY}
LDADD=	-lskey

beforeinstall:
	install -c -m 755 ${.CURDIR}/keyaudit.sh ${BINDIR}/keyaudit
	install -c -m 755 ${.CURDIR}/keyinfo.sh ${BINDIR}/keyinfo

.include <bsd.prog.mk>

# $Id: Makefile,v 1.3 1994/06/24 08:06:23 deraadt Exp $

PROG= skey
CFLAGS+= -I${.CURDIR}/../../lib/libskey
MAN1= skey.0 skeyinfo.0 skeyaudit.0
DPADD=	${LIBSKEY}
LDADD=	-lskey

beforeinstall:
	install -c -m 755 ${.CURDIR}/skeyaudit.sh ${BINDIR}/skeyaudit
	install -c -m 755 ${.CURDIR}/skeyinfo.sh ${BINDIR}/skeyinfo

.include <bsd.prog.mk>

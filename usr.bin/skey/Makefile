# $Id: Makefile,v 1.4 1994/06/27 04:53:04 jtc Exp $

PROG= skey
CFLAGS+= -I${.CURDIR}/../../lib/libskey
MAN1= skey.0 skeyinfo.0 skeyaudit.0
DPADD=	${LIBSKEY}
LDADD=	-lskey

beforeinstall:
	install -c -m 755 ${.CURDIR}/skeyaudit.sh ${DESTDIR}${BINDIR}/skeyaudit
	install -c -m 755 ${.CURDIR}/skeyinfo.sh ${DESTDIR}${BINDIR}/skeyinfo

.include <bsd.prog.mk>

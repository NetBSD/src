# $Id: Makefile,v 1.5 1994/12/22 12:30:55 cgd Exp $

PROG= skey
CFLAGS+= -I${.CURDIR}/../../lib/libskey
MAN= skey.1 skeyinfo.1 skeyaudit.1
DPADD=	${LIBSKEY}
LDADD=	-lskey

beforeinstall:
	install -c -m 755 ${.CURDIR}/skeyaudit.sh ${DESTDIR}${BINDIR}/skeyaudit
	install -c -m 755 ${.CURDIR}/skeyinfo.sh ${DESTDIR}${BINDIR}/skeyinfo

.include <bsd.prog.mk>

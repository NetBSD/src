#	$NetBSD: Makefile,v 1.8 1997/03/24 21:59:49 christos Exp $

PROG= skey
CFLAGS+= -I${.CURDIR}/../../lib/libskey
MAN= skey.1 skeyinfo.1 skeyaudit.1
DPADD=	${LIBSKEY}
LDADD=	-lskey
SCRIPTS=skeyaudit.sh skeyinfo.sh

.include <bsd.prog.mk>

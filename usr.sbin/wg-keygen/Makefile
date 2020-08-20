#	$NetBSD: Makefile,v 1.1 2020/08/20 21:28:02 riastradh Exp $

PROG=	wg-keygen
MAN=	wg-keygen.8

DPADD+=	${LIBSSH}
LDADD+=	-lssh

.include <bsd.prog.mk>

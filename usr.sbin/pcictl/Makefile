#	$NetBSD: Makefile,v 1.1 2001/09/13 23:51:04 thorpej Exp $

PROG=	pcictl
MAN=	pcictl.8

DPADD+=	${LIBPCI} ${LIBUTIL}
LDADD+=	-lpci -lutil

.include <bsd.prog.mk>

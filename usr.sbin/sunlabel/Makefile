#	$NetBSD: Makefile,v 1.2 2002/01/15 00:57:27 wiz Exp $

PROG=	sunlabel
MAN=	sunlabel.8

LDADD+=	-ltermcap -lutil
DPADD+=	${LIBTERMCAP} ${LIBUTIL}

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif

#	$NetBSD: Makefile,v 1.2.2.1 2004/07/23 14:33:15 tron Exp $

PROG=	sunlabel
MAN=	sunlabel.8

.ifndef HOSTPROG
LDADD+=	-ltermcap
DPADD+=	${LIBTERMCAP}

DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.include <bsd.prog.mk>
.endif

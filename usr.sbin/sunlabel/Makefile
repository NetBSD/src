#	$NetBSD: Makefile,v 1.4 2003/05/14 00:24:07 dbj Exp $

PROG=	sunlabel
MAN=	sunlabel.8

LDADD+=	-ltermcap
.ifndef HOSTPROG
DPADD+=	${LIBTERMCAP}

DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.include <bsd.prog.mk>
.endif

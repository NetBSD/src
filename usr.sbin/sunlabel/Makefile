#	$NetBSD: Makefile,v 1.6 2003/11/12 02:17:53 matt Exp $

PROG=	sunlabel
MAN=	sunlabel.8

.ifndef HOSTPROG
LDADD+=	-ltermcap
DPADD+=	${LIBTERMCAP}

DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.endif

.include <bsd.prog.mk>

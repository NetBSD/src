#	$NetBSD: Makefile,v 1.5 2003/05/18 07:57:40 lukem Exp $

PROG=	sunlabel
MAN=	sunlabel.8

LDADD+=	-ltermcap
.ifndef HOSTPROG
DPADD+=	${LIBTERMCAP}

DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.endif

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.7 2010/02/03 15:34:47 roy Exp $

PROG=	sunlabel
MAN=	sunlabel.8

.ifndef HOSTPROG
LDADD+=	-lterminfo
DPADD+=	${LIBTERMINFO}

DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.endif

.include <bsd.prog.mk>

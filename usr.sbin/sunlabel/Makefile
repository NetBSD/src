#	$NetBSD: Makefile,v 1.3 2003/03/13 04:50:53 thorpej Exp $

PROG=	sunlabel
MAN=	sunlabel.8

LDADD+=	-ltermcap
DPADD+=	${LIBTERMCAP}

.ifndef HOSTPROG
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.include <bsd.prog.mk>
.endif

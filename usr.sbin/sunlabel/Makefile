#	$NetBSD: Makefile,v 1.1 2002/01/10 16:24:41 mrg Exp $

PROG=	sunlabel
NOMAN=	# sunlabel.8

LDADD+=	-ltermcap -lutil
DPADD+=	${LIBTERMCAP} ${LIBUTIL}

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif

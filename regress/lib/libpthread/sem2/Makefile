#	$NetBSD: Makefile,v 1.1 2004/11/03 15:18:35 yamt Exp $

WARNS=2

PROG=   sem2

LDADD+=	-lpthread
LDADD+=	-lrt

NOMAN=

regress:
	./${PROG}

.include <bsd.prog.mk>

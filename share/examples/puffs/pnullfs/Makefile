#	$NetBSD: Makefile,v 1.2 2016/08/26 22:12:22 tnn Exp $

PROG=		 pnullfs
CPPFLAGS+=	-D_KERNTYPES
LDADD+=		-lpuffs -lutil
DBG=		-g -O0

WARNS=	2

NOMAN=	1

.include <bsd.prog.mk>

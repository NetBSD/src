#	$NetBSD: Makefile,v 1.2 2016/01/23 21:22:47 christos Exp $
#

PROG=	mount_puffs

DPADD+=	${LIBUTIL}
LDADD+=	-lutil
CPPFLAGS+=	-D_KERNTYPES

MAN=	mount_puffs.8

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.1 2010/05/01 23:31:01 pooka Exp $
#

PROG=	pad_nothai

LDADD+=	-lrumpdev_audio -lrumpdev_pad -lrumpdev -lrumpvfs
LDADD+=	-lrump
LDADD+=	-lrumpuser -lpthread

DBG=	-g
NOMAN=	
WARNS=	4

.include <bsd.prog.mk>

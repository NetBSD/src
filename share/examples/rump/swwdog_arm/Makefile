#	$NetBSD: Makefile,v 1.1 2010/01/31 03:11:55 pooka Exp $
#

PROG=	swwdog_arm

LDADD+=	-lrumpdev_sysmon -lrumpdev
LDADD+=	-lrumpvfs -lrump
LDADD+=	-lrumpuser -lpthread

DBG=	-g
NOMAN=	
WARNS=	4

.include <bsd.prog.mk>

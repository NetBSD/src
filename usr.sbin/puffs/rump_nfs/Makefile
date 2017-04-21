#	$NetBSD: Makefile,v 1.6.32.1 2017/04/21 16:54:18 bouyer Exp $
#

.include <bsd.own.mk>

MOUNTNAME=	nfs

SRCS+=		getnfsargs.c
LDADD+= 	-lrumpfs_nfs -lrumpdev -lrumpnet_net -lrumpnet -lrumpnet_sockin

ISRUMP=		# gives me nightmares
#CPPFLAGS+=	-DINET6 # for getnfsargs?

.include <bsd.prog.mk>

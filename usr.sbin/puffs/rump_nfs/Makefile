#	$NetBSD: Makefile,v 1.6.28.1 2017/03/20 06:58:09 pgoyette Exp $
#

.include <bsd.own.mk>

MOUNTNAME=	nfs

SRCS+=		getnfsargs.c
LDADD+= 	-lrumpfs_nfs -lrumpdev -lrumpnet_net -lrumpnet -lrumpnet_sockin

ISRUMP=		# gives me nightmares
#CPPFLAGS+=	-DINET6 # for getnfsargs?

.include <bsd.prog.mk>

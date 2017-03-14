#	$NetBSD: Makefile,v 1.7 2017/03/14 14:05:52 ozaki-r Exp $
#

.include <bsd.own.mk>

MOUNTNAME=	nfs

SRCS+=		getnfsargs.c
LDADD+= 	-lrumpfs_nfs -lrumpdev -lrumpnet_net -lrumpnet -lrumpnet_sockin

ISRUMP=		# gives me nightmares
#CPPFLAGS+=	-DINET6 # for getnfsargs?

.include <bsd.prog.mk>

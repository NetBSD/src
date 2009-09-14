#	$NetBSD: Makefile,v 1.5 2009/09/14 17:24:20 skrll Exp $
#

.include <bsd.own.mk>

MOUNTNAME=	nfs

SRCS+=		getnfsargs.c

# One or the other of the following.  Compile-time selection for now.
# The first one uses host sockets to provide a networking stack, the
# second one runs the real NetBSD kernel networking stack in userspace
# and uses /dev/tap to access the network.
#
.ifdef RUMPKMOD
LDADD+= 	-lrumpnet -lrumpnet_sockin
.else
LDADD+= 	-lrumpfs_nfs -lrumpnet -lrumpnet_sockin
.endif
#LDADD+= 	-lrumpfs_nfs -lrumpnet -lrumpnet_net -lrumpnet_netinet	\
		-lrumpnet_virtif

ISRUMP=		# gives me nightmares
#CPPFLAGS+=	-DINET6 # for getnfsargs?

.include <bsd.prog.mk>

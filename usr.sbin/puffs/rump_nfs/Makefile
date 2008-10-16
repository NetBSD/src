#	$NetBSD: Makefile,v 1.1 2008/10/16 09:30:57 pooka Exp $
#

MOUNTNAME=	nfs

SRCS+=		getnfsargs.c

# One or the other of the following.  Compile-time selection for now.
# The first one uses host sockets to provide a networking stack, the
# second one runs the real NetBSD kernel networking stack in userspace
# and uses /dev/tap to access the network.
#
LDADD+= 	-lrumpfs_nfs -lrumpnet -lrumpnet_sockin
#LDADD+= 	-lrumpfs_nfs -lrumpnet -lrumpnet_net -lrumpnet_netinet	\
		-lrumpnet_virtif

ISRUMP=		# gives me nightmares
#CPPFLAGS+=	-DINET6 # for getnfsargs?

.include <bsd.prog.mk>

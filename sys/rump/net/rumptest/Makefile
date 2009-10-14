#	$NetBSD: Makefile,v 1.6 2009/10/14 17:29:20 pooka Exp $
#

PROG=	rumptest_net
NOMAN=	woman
WARNS=4

# run real tcp/ip stack in userspace
LDADD=	-lrump -lrumpnet -lrumpnet_net -lrumpnet_netinet -lrumpnet_virtif \
	-lrumpuser -lpthread
CPPFLAGS+=	-DFULL_NETWORK_STACK

# use protocol families provided by host kernel sockets
#LDADD=	-lrump -lrumpnet -lrumpnet_sockin -lrumpuser -lpthread

LDADD+= -lrumpvfs

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.3.4.1 2009/01/19 13:20:29 skrll Exp $
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

.include <bsd.prog.mk>

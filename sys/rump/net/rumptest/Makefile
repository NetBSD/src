#	$NetBSD: Makefile,v 1.3 2008/10/16 19:28:26 pooka Exp $
#

PROG=	rumptest_net
NOMAN=	woman

# run real tcp/ip stack in userspace
LDADD=	-lrump -lrumpnet -lrumpnet_net -lrumpnet_inet -lrumpnet_virtif	\
	-lrumpuser -lpthread
CPPFLAGS+=	-DFULL_NETWORK_STACK

# use protocol families provided by host kernel sockets
#LDADD=	-lrump -lrumpnet -lrumpnet_sockin -lrumpuser -lpthread

.include <bsd.prog.mk>

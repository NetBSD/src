#	$NetBSD: Makefile,v 1.1 2008/10/14 00:56:24 pooka Exp $
#

PROG=	rumptest_net
NOMAN=	woman

# run real tcp/ip stack in userspace
LDADD=-lrump -lrumpnet_net -lrumpnet_inet -lrumpnet_virtif -lrumpuser -lpthread

# use protocol families provided by host kernel sockets
#LDADD=	-lrump -lrumpnet_sockin -lrumpuser -lpthread

.include <bsd.prog.mk>

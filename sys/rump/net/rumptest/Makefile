#	$NetBSD: Makefile,v 1.2 2008/10/15 13:11:45 pooka Exp $
#

PROG=	rumptest_net
NOMAN=	woman

# run real tcp/ip stack in userspace
LDADD=	-lrump -lrumpnet -lrumpnet_net -lrumpnet_inet -lrumpnet_virtif	\
	-lrumpuser -lpthread

# use protocol families provided by host kernel sockets
#LDADD=	-lrump -lrumpnet -lrumpnet_sockin -lrumpuser -lpthread

.include <bsd.prog.mk>

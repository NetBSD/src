#	$NetBSD: Makefile,v 1.9 2010/01/25 22:26:19 pooka Exp $
#

PROG=	rumptest_net
NOMAN=	woman
WARNS=4

# run real tcp/ip stack in userspace
LDADD=	-lrump -lrumpnet -lrumpnet_net -lrumpnet_netinet -lrumpnet_virtif \
	-lrumpuser -lpthread
CPPFLAGS+=	-DFULL_NETWORK_STACK
# for loading bpf module
LDADD+=	-lrumpvfs

# uncomment for bpf support
#LDADD+= 	-lrumpdev_bpf -lrumpdev -lrumpvfs

# use protocol families provided by host kernel sockets
#LDADD=	-lrump -lrumpnet -lrumpnet_sockin -lrumpuser -lpthread

.include <bsd.prog.mk>

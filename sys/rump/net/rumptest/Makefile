#	$NetBSD: Makefile,v 1.10 2010/01/26 17:52:21 pooka Exp $
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

# for loading bpf module
LDADD+=	-lrumpvfs

# uncomment for bpf support (there's kmod support too, though)
#LDADD+= 	-lrumpdev_bpf -lrumpdev -lrumpvfs

.include <bsd.prog.mk>

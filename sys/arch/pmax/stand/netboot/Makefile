# $NetBSD: Makefile,v 1.3 2005/12/11 12:18:41 christos Exp $

SECONDARY_PROG=	netboot
STRIPFLAG=

SRCS=		start.S boot.c
SRCS+=		bootinfo.c callvec.c clock.c conf.c devopen.c dev_net.c
SRCS+=		if_prom.c putchar.c rz.c
SRCS+=		bootinit.S bootread.S getchar.S startprog.S

CPPFLAGS+=	-DBOOT_TYPE_NAME='"Netboot"' \
		-DBOOTNET -DSUPPORT_BOOTP -DSUPPORT_DHCP -DFILL_ARPCACHE

#CPPFLAGS+=	-DBOOTP_DEBUG -DNETIF_DEBUG -DETHER_DEBUG -DNFS_DEBUG \
#		-DRPC_DEBUG -DNET_DEBUG -DDEBUG -DPARANOID -DARP_DEBUG \
#		-DDEBUG_VAL=1

DO_ECOFF_BINARY=1

.include "../Makefile.booters"

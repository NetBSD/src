#	$Id: Makefile,v 1.4 1993/10/13 16:33:55 cgd Exp $

LIB=	netboot
SRCS=	arp.c bootinfo.c bootp.c exec.c globals.c in_cksum.c \
	net.c netif.c nfsboot.c rpc.c util.c
CFLAGS+= -DCOMMON_ETHERADDR

NOPROFILE=
NOPIC=

install:

.include <bsd.lib.mk>

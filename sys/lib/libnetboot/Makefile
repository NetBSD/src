#	$Id: Makefile,v 1.3 1993/10/13 11:31:32 cgd Exp $

LIB=	netboot
SRCS=	arp.c bootp.c exec.c in_cksum.c net.c netif.c nfsboot.c \
	rpc.c util.c
CFLAGS+= -DCOMMON_ETHERADDR

NOPROFILE=
NOPIC=

install:

.include <bsd.lib.mk>

#	$Id: Makefile,v 1.9 1994/06/14 00:31:43 glass Exp $

LIB=	netboot
SRCS+=	arp.c bootinfo.c bootp.c exec.c globals.c in_cksum.c \
	net.c netif.c rpc.c util.c

SRCS+=	boot_nfs.c boot_tftp.c

CFLAGS+=	-DDEBUG
CFLAGS+=	-DPARANOID

CFLAGS+=	-I${.CURDIR}/../libc_sa

NOPROFILE=
NOPIC=
OBJMACHINE=

install:

.include <bsd.lib.mk>

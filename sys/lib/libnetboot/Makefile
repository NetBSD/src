#	$Id: Makefile,v 1.8 1993/10/23 02:33:48 cgd Exp $

LIB=	netboot
SRCS+=	arp.c bootinfo.c bootp.c exec.c globals.c in_cksum.c \
	net.c netif.c rpc.c util.c

SRCS+=	boot_nfs.c boot_tftp.c

CFLAGS+=	-DDEBUG
CFLAGS+=	-DPARANOID -Wall

CFLAGS+=	-I${.CURDIR}/../libc_sa

NOPROFILE=
NOPIC=
OBJMACHINE=

install:

.include <bsd.lib.mk>

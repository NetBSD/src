#	$Id: Makefile,v 1.7 1993/10/16 07:57:36 cgd Exp $

LIB=	netboot
SRCS+=	arp.c bootinfo.c bootp.c exec.c globals.c in_cksum.c \
	net.c netif.c rpc.c util.c

SRCS+=	boot_nfs.c boot_tftp.c

CFLAGS+=	-DDEBUG
CFLAGS+=	-DPARANOID -Wall

CFLAGS+=	-I${.CURDIR}/../libc_sa

NOPROFILE=
NOPIC=

install:

.include <bsd.lib.mk>

#	$NetBSD: Makefile,v 1.10 1994/10/26 06:42:58 cgd Exp $

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

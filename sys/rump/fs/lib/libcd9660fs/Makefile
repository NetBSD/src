#	$NetBSD: Makefile,v 1.2.14.2 2007/11/02 12:43:33 joerg Exp $
#

.include <bsd.own.mk>

LIB=	cd9660fs

.PATH:	${NETBSDSRCDIR}/sys/fs/cd9660

SRCS=	cd9660_bmap.c cd9660_node.c cd9660_util.c cd9660_vnops.c	\
	cd9660_lookup.c cd9660_rrip.c cd9660_vfsops.c

CFLAGS+=-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

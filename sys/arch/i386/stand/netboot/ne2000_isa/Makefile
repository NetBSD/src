#	$NetBSD: Makefile,v 1.4 2012/08/10 12:18:15 joerg Exp $

USE_NETIF=	ne2000_isa

CPPFLAGS+= -DBASEREG=0x300

.include "../Makefile.netboot"

COPTS.dp8390.c +=	-Wno-pointer-sign


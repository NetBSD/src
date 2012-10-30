#	$NetBSD: Makefile,v 1.3.2.1 2012/10/30 17:19:52 yamt Exp $

USE_NETIF=	ne2000_isa

CPPFLAGS+= -DBASEREG=0x300

.include "../Makefile.netboot"

COPTS.dp8390.c +=	-Wno-pointer-sign


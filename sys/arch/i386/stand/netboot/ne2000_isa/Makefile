#	$NetBSD: Makefile,v 1.2.8.1 2011/08/27 15:37:28 jym Exp $

USE_NETIF=	ne2000_isa

CPPFLAGS+= -DBASEREG=0x300

.include "../Makefile.netboot"

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.dp8390.c +=	-Wno-pointer-sign
.endif


#	$NetBSD: Makefile,v 1.3 2011/06/20 08:46:28 mrg Exp $

USE_NETIF=	ne2000_isa

CPPFLAGS+= -DBASEREG=0x300

.include "../Makefile.netboot"

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.dp8390.c +=	-Wno-pointer-sign
.endif


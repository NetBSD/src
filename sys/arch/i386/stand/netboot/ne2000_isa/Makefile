#	$NetBSD: Makefile,v 1.2.26.1 2011/06/23 14:19:17 cherry Exp $

USE_NETIF=	ne2000_isa

CPPFLAGS+= -DBASEREG=0x300

.include "../Makefile.netboot"

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.dp8390.c +=	-Wno-pointer-sign
.endif


#	$NetBSD: Makefile,v 1.1.134.1 2008/10/19 22:15:50 haad Exp $

USE_NETIF=	ne2000_isa

CPPFLAGS+= -DBASEREG=0x300

.include "../Makefile.netboot"

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.dp8390.c +=	-Wno-pointer-sign
.endif


#	$NetBSD: Makefile,v 1.1.130.1 2008/09/18 04:33:28 wrstuden Exp $

USE_NETIF=	ne2000_isa

CPPFLAGS+= -DBASEREG=0x300

.include "../Makefile.netboot"

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.dp8390.c +=	-Wno-pointer-sign
.endif


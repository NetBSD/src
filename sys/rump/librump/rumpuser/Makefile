#	$NetBSD: Makefile,v 1.12 2008/11/17 08:53:12 pooka Exp $
#

LIB=		rumpuser
WARNS=		4

SRCS=		rumpuser.c rumpuser_net.c rumpuser_pth.c

LDFLAGS+=	-Wl,--wrap=malloc

# no shlib_version because this is automatically in sync with lib/librumpuser
SHLIB_MAJOR=	0
SHLIB_MINOR=	0

.include <bsd.lib.mk>

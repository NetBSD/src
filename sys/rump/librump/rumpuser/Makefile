#	$NetBSD: Makefile,v 1.7.4.1 2008/10/19 22:18:07 haad Exp $
#

LIB=		rumpuser
INCS=		rumpuser.h
INCSDIR=	/usr/include/rump
WARNS=		4

SRCS=		rumpuser.c rumpuser_net.c rumpuser_pth.c

LDFLAGS+=	-Wl,--wrap=malloc

# no shlib_version because this is automatically in sync with lib/librumpuser
SHLIB_MAJOR=	0
SHLIB_MINOR=	0

.include <bsd.lib.mk>

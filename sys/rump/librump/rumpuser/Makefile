#	$NetBSD: Makefile,v 1.5.6.4 2008/10/05 20:11:34 mjf Exp $
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

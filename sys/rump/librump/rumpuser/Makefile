#	$NetBSD: Makefile,v 1.6.6.2 2008/09/18 04:37:04 wrstuden Exp $
#

LIB=		rumpuser
INCS=		rumpuser.h
INCSDIR=	/usr/include/rump

SRCS=		rumpuser.c rumpuser_pth.c

LDFLAGS+=	-Wl,--wrap=malloc

# no shlib_version because this is automatically in sync with lib/librumpuser
SHLIB_MAJOR=	0
SHLIB_MINOR=	0

.include <bsd.lib.mk>

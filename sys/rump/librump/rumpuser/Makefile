#	$NetBSD: Makefile,v 1.7.2.1 2008/07/31 04:51:05 simonb Exp $
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

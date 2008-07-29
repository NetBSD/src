#	$NetBSD: Makefile,v 1.8 2008/07/29 13:17:47 pooka Exp $
#

.PATH:		${NETBSDSRCDIR}/sys/rump/librump/librumpuser

LIB=		rumpuser
INCS=		rumpuser.h
INCSDIR=	/usr/include/rump

SRCS=		rumpuser.c rumpuser_pth.c

# no shlib_version because this is automatically in sync with lib/librumpuser
SHLIB_MAJOR=	0
SHLIB_MINOR=	0

.include <bsd.lib.mk>

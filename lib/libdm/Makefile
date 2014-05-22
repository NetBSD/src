# $NetBSD: Makefile,v 1.1.6.2 2014/05/22 11:36:55 yamt Exp $
USE_FORT?= no  # network protocol library

LIB=    dm

SRCS=   libdm_ioctl.c
MAN=	dm.3

WARNS= 2

CPPFLAGS+= -I${.CURDIR}
#CFLAGS+= -g -O0
INCS= dm.h
INCSDIR= /usr/include

LIBDPLIBS+=	prop		${NETBSDSRCDIR}/lib/libprop

.ifdef RUMP_ACTION
CPPFLAGS+= -DRUMP_ACTION
.endif

.include <bsd.lib.mk>

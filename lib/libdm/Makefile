# $NetBSD: Makefile,v 1.2 2012/03/21 05:36:24 matt Exp $
USE_FORT?= no  # network protocol library

LIB=    dm

SRCS=   libdm_ioctl.c
MAN=	dm.3

WARNS= 2

CPPFLAGS+= -I${.CURDIR}
#CFLAGS+= -g -O0
INCS= dm.h
INCSDIR= /usr/include

#LDADD+= -lprop
#DPADD+= ${LIBPROP}

.ifdef RUMP_ACTION
CPPFLAGS+= -DRUMP_ACTION
.endif

.include <bsd.lib.mk>

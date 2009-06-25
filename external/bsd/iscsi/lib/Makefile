# $NetBSD: Makefile,v 1.2 2009/06/25 13:47:12 agc Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network protocol library

LIB= iscsi
SRCS= conffile.c disk.c iscsi.c util.c parameters.c storage.c
SRCS+= netmask.c md5c.c md5hl.c uuid.c
SRCS+= initiator.c target.c 
CPPFLAGS+= -DCONFIG_ISCSI_DEBUG -DHAVE_CONFIG_H
CPPFLAGS+= -I${ISCSIDIST}/include
CPPFLAGS+= -pthread
LDFLAGS+= -pthread
MAN= libiscsi.3
WARNS=4

INCS+= iscsi.h
INCSDIR=/usr/include

ISCSIDIST=	${.CURDIR}/../dist
.PATH:	${ISCSIDIST}/src/lib ${ISCSIDIST}/include

LIBDPLIBS+=      pthread  ${.CURDIR}/../../../../lib/libpthread

.include <bsd.lib.mk>

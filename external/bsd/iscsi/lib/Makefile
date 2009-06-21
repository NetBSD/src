# $NetBSD: Makefile,v 1.1 2009/06/21 21:11:16 agc Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network protocol library

LIB= iscsi
SRCS= conffile.c disk.c target.c iscsi.c util.c parameters.c storage.c
SRCS+= netmask.c md5c.c md5hl.c
CPPFLAGS+= -DCONFIG_ISCSI_DEBUG -DHAVE_CONFIG_H
CPPFLAGS+= -I${ISCSI_DIST}/include
CPPFLAGS+= -pthread
LDFLAGS+= -pthread
NOMAN= # defined
WARNS=4

ISCSI_DIST=	${.CURDIR}/../dist
.PATH:	${ISCSI_DIST}/src

LIBDPLIBS+=      pthread  ${NETBSDSRCDIR}/lib/libpthread

.include <bsd.lib.mk>

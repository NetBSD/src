# $NetBSD: Makefile,v 1.8 2009/04/18 16:31:30 tsutsui Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network protocol library

LIB= iscsi
SRCS= conffile.c disk.c target.c iscsi.c util.c parameters.c storage.c
SRCS+= netmask.c md5c.c md5hl.c
CPPFLAGS+= -DCONFIG_ISCSI_DEBUG -DHAVE_CONFIG_H
CPPFLAGS+= -I${ISCSIDIST}/include
CPPFLAGS+= -pthread
LDFLAGS+= -pthread
NOMAN= # defined
WARNS=4

ISCSIDIST=	${NETBSDSRCDIR}/dist/iscsi
.PATH:	${ISCSIDIST}/src

LIBDPLIBS+=      pthread  ${.CURDIR}/../libpthread

.include <bsd.lib.mk>

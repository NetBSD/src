# $NetBSD: Makefile,v 1.10 2009/06/25 20:47:45 dyoung Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network protocol library

LIB= iscsi
SRCS= conffile.c disk.c target.c iscsi.c util.c parameters.c storage.c
SRCS+= netmask.c md5c.c md5hl.c uuid.c
CPPFLAGS+= -DCONFIG_ISCSI_DEBUG -DHAVE_CONFIG_H
CPPFLAGS+= -I${ISCSIDIST}/include
CPPFLAGS+= -pthread
LDFLAGS+= -pthread
NOMAN= # defined
WARNS=4

ISCSIDIST=	${NETBSDSRCDIR}/external/bsd/iscsi/dist
.PATH:	${ISCSIDIST}/src

LIBDPLIBS+=      pthread  ${.CURDIR}/../libpthread

.include <bsd.lib.mk>

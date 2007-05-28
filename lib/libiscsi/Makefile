# $NetBSD: Makefile,v 1.4 2007/05/28 12:06:19 tls Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network protocol library

LIB= iscsi
SRCS= conffile.c disk.c target.c iscsi.c util.c parameters.c storage.c netmask.c
CPPFLAGS+= -DCONFIG_ISCSI_DEBUG -D_FILE_OFFSET_BITS=64
CPPFLAGS+= -I${ISCSIDIST}/include
CPPFLAGS+= -pthread
LDFLAGS+= -pthread
NOMAN= # defined
WARNS=4

ISCSIDIST=	${NETBSDSRCDIR}/dist/iscsi
.PATH:	${ISCSIDIST}/src

LIBDPLIBS+=      pthread  ${.CURDIR}/../libpthread

.include <bsd.lib.mk>

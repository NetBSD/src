# $NetBSD: Makefile,v 1.8 2025/09/18 03:35:59 mrg Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network protocol library 

LIB= iscsi
SRCS= conffile.c disk.c util.c parameters.c protocol.c storage.c
SRCS+= netmask.c md5c.c md5hl.c crc32.c uuid.c
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

COPTS.conffile.c+=	${CC_WNO_CALLOC_TRANSPOSED_ARGS}
COPTS.disk.c+=		${CC_WNO_CALLOC_TRANSPOSED_ARGS}
COPTS.initiator.c+=	${CC_WNO_CALLOC_TRANSPOSED_ARGS}
COPTS.storage.c+=	${CC_WNO_CALLOC_TRANSPOSED_ARGS}
COPTS.target.c+=	${CC_WNO_FORMAT_TRUNCATION} ${CC_WNO_CALLOC_TRANSPOSED_ARGS}

.include <bsd.lib.mk>

# $NetBSD: Makefile,v 1.2 2006/02/09 23:09:06 agc Exp $

.include <bsd.own.mk>

LIB= iscsi
SRCS= conffile.c disk.c target.c iscsi.c util.c parameters.c storage.c
CPPFLAGS+= -DCONFIG_ISCSI_DEBUG -D_FILE_OFFSET_BITS=64
CPPFLAGS+= -I${ISCSIDIST}/include
CPPFLAGS+= -pthread
LDFLAGS+= -pthread
NOMAN= # defined
WARNS=4

ISCSIDIST=	${NETBSDSRCDIR}/dist/iscsi
.PATH:	${ISCSIDIST}/src

LIBDPLIBS=      pthread  ${.CURDIR}/../libpthread

.include <bsd.lib.mk>

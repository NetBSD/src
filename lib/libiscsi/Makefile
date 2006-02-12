# $NetBSD: Makefile,v 1.3 2006/02/12 15:38:34 agc Exp $

.include <bsd.own.mk>

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

LIBDPLIBS=      pthread  ${.CURDIR}/../libpthread

.include <bsd.lib.mk>

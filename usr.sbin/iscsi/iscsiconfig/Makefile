# $NetBSD: Makefile,v 1.2 2009/04/22 15:23:04 lukem Exp $

.include <bsd.own.mk>

DIST=	${.CURDIR}/../../../dist/iscsi
.PATH:	${DIST}/src

PROG= iscsiconfig
SRCS= tests.c initiator.c osd_ops.c iscsiconfig.c
CPPFLAGS+= -DCONFIG_ISCSI_DEBUG -D_FILE_OFFSET_BITS=64
CPPFLAGS+= -I${DIST}/include -I${.CURDIR}/../include
CPPFLAGS+= -pthread
LDADD+=  -L${LIBDIR} -Wl,-R${LIBDIR} -liscsi
LDFLAGS+= -pthread
MAN= iscsiconfig.8

.include <bsd.prog.mk>

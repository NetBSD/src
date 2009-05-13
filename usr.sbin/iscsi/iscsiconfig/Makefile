# $NetBSD: Makefile,v 1.1.28.1 2009/05/13 19:20:25 jym Exp $

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

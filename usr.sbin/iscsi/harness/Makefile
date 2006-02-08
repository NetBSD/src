# $NetBSD: Makefile,v 1.1 2006/02/08 18:57:49 agc Exp $

.include <bsd.own.mk>

DIST=	${.CURDIR}/../../../dist/iscsi
.PATH:	${DIST}/src

PROG= iscsi-harness
SRCS= tests.c osd_ops.c initiator.c iscsi-harness.c
CPPFLAGS+= -DCONFIG_ISCSI_DEBUG -D_FILE_OFFSET_BITS=64
CPPFLAGS+= -I${DIST}/include -I${.CURDIR}/../include
CPPFLAGS+= -pthread
LDADD+=  -L${LIBDIR} -Wl,-R${LIBDIR} -liscsi
LDFLAGS+= -pthread
MAN= iscsi-harness.8
WARNS=4

.include <bsd.prog.mk>

# $NetBSD: Makefile,v 1.3 2009/04/22 15:23:04 lukem Exp $

.sinclude "${.CURDIR}/../../Makefile.inc"

.include <bsd.own.mk>

DIST=	${.CURDIR}/../../../dist/iscsi
.PATH:	${DIST}/src

PROG= iscsi-harness
SRCS= tests.c osd_ops.c initiator.c iscsi-harness.c
CPPFLAGS+= -DCONFIG_ISCSI_DEBUG
CPPFLAGS+= -I${DIST}/include -I${.CURDIR}/../include
CPPFLAGS+= -pthread
LDADD+=  -L${LIBDIR} -Wl,-R${LIBDIR} -liscsi
LDFLAGS+= -pthread
MAN= iscsi-harness.8

.include <bsd.prog.mk>

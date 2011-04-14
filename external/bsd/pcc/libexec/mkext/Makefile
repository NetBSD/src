#	$NetBSD: Makefile,v 1.1 2011/04/14 09:31:25 plunky Exp $

#
# this is really part of ccom, but HOSTPROG and PROG don't mix
#

.include <bsd.init.mk>

.PATH:	${PCC_DIST}/cc/ccom \
	${PCC_DIST}/arch/${TARGMACH} \
	${PCC_DIST}/mip

HOSTPROG=	mkext
MAN=
SRCS=		mkext.c table.c common.c

HOST_CPPFLAGS+=	-DMKEXT
HOST_CPPFLAGS+=	-I${PCC_DIST}/mip
HOST_CPPFLAGS+=	-I${PCC_DIST}/arch/${TARGMACH}
HOST_CPPFLAGS+=	-I${PCC_DIR}/include

.include <bsd.hostprog.mk>

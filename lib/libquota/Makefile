#	$NetBSD: Makefile,v 1.2 2012/01/09 15:22:38 dholland Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

.include "${NETBSDSRCDIR}/common/lib/libquota/Makefile.inc"

WARNS=	4
LIB=	quota

LIBDPLIBS+=     prop  ${.CURDIR}/../libprop
LIBDPLIBS+=     rpcsvc  ${.CURDIR}/../librpcsvc

SRCS+=	getfsquota.c getnfsquota.c getufsquota.c
MAN=

SRCS+=	quota_open.c
SRCS+=	quota_schema.c
SRCS+=	quota_get.c quota_put.c quota_delete.c
SRCS+=	quota_cursor.c

.include <bsd.lib.mk>

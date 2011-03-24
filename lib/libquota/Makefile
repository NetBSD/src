#	$NetBSD: Makefile,v 1.1 2011/03/24 17:05:43 bouyer Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

.include "${NETBSDSRCDIR}/common/lib/libquota/Makefile.inc"

WARNS=	4
LIB=	quota

LIBDPLIBS+=     prop  ${.CURDIR}/../libprop
LIBDPLIBS+=     rpcsvc  ${.CURDIR}/../librpcsvc

SRCS+=	getfsquota.c getnfsquota.c getufsquota.c
MAN=

.include <bsd.lib.mk>

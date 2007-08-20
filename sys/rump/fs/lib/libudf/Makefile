#	$NetBSD: Makefile,v 1.1.4.2 2007/08/20 22:07:22 ad Exp $
#

.include <bsd.own.mk>

LIB=	udf

.PATH:  ${NETBSDSRCDIR}/sys/fs/udf

SRCS=	udf_osta.c udf_subr.c udf_vfsops.c udf_vnops.c

CFLAGS+=	-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

#	$NetBSD: Makefile,v 1.1.8.2 2007/09/03 14:45:15 yamt Exp $
#

.include <bsd.own.mk>

LIB=	udf

.PATH:  ${NETBSDSRCDIR}/sys/fs/udf

SRCS=	udf_osta.c udf_subr.c udf_vfsops.c udf_vnops.c

CFLAGS+=	-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

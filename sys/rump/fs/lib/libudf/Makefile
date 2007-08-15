#	$NetBSD: Makefile,v 1.1.2.2 2007/08/15 13:50:29 skrll Exp $
#

.include <bsd.own.mk>

LIB=	udf

.PATH:  ${NETBSDSRCDIR}/sys/fs/udf

SRCS=	udf_osta.c udf_subr.c udf_vfsops.c udf_vnops.c

CFLAGS+=	-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

#	$NetBSD: Makefile,v 1.1.6.2 2009/01/19 13:20:10 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/nullfs ${S}/miscfs/genfs

KMOD=	null
SRCS=	null_vfsops.c null_vnops.c

.include <bsd.kmodule.mk>

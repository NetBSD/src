#	$NetBSD: Makefile,v 1.1.10.2 2009/05/04 08:14:11 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/nullfs ${S}/miscfs/genfs

KMOD=	null
SRCS=	null_vfsops.c null_vnops.c

.include <bsd.kmodule.mk>

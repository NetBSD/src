#	$NetBSD: Makefile,v 1.1.2.2 2008/06/29 09:33:17 mjf Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/nullfs ${S}/miscfs/genfs

KMOD=	nullfs
SRCS=	null_vfsops.c null_vnops.c

.include <bsd.kmodule.mk>

#	$NetBSD: Makefile,v 1.1 2008/06/28 16:11:36 rumble Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/nullfs ${S}/miscfs/genfs

KMOD=	nullfs
SRCS=	null_vfsops.c null_vnops.c

.include <bsd.kmodule.mk>

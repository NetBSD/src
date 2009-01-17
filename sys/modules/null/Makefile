#	$NetBSD: Makefile,v 1.1.4.2 2009/01/17 13:29:30 mjf Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/nullfs ${S}/miscfs/genfs

KMOD=	null
SRCS=	null_vfsops.c null_vnops.c

.include <bsd.kmodule.mk>

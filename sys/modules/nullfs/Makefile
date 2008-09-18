#	$NetBSD: Makefile,v 1.1.8.2 2008/09/18 04:36:59 wrstuden Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/nullfs ${S}/miscfs/genfs

KMOD=	nullfs
SRCS=	null_vfsops.c null_vnops.c

.include <bsd.kmodule.mk>

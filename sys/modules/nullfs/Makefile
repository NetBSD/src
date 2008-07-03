#	$NetBSD: Makefile,v 1.1.4.2 2008/07/03 18:38:20 simonb Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/nullfs ${S}/miscfs/genfs

KMOD=	nullfs
SRCS=	null_vfsops.c null_vnops.c

.include <bsd.kmodule.mk>

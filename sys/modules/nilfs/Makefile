#	$NetBSD: Makefile,v 1.1.4.2 2009/08/19 18:48:22 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/nilfs

KMOD=  	nilfs
SRCS=	nilfs_vfsops.c nilfs_vnops.c nilfs_subr.c

.include <bsd.kmodule.mk>

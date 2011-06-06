#	$NetBSD: Makefile,v 1.1.10.1 2011/06/06 09:09:49 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/nilfs

KMOD=  	nilfs
SRCS=	nilfs_vfsops.c nilfs_vnops.c nilfs_subr.c

CWARNFLAGS.clang=	-Wno-self-assign

.include <bsd.kmodule.mk>

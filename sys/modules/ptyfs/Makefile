#	$NetBSD: Makefile,v 1.3 2019/09/26 18:36:08 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/ptyfs ${S}/ ${S}/arch/${MACHINE}/${MACHINE} \
	${S}/arch/${MACHINE_CPU}/${MACHINE_CPU}

KMOD=	ptyfs
SRCS=	ptyfs_vfsops.c ptyfs_vnops.c ptyfs_subr.c

.include <bsd.kmodule.mk>

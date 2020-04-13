#	$NetBSD: Makefile,v 1.1.92.2 2020/04/13 08:05:13 martin Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/ptyfs ${S}/ ${S}/arch/${MACHINE}/${MACHINE} \
	${S}/arch/${MACHINE_CPU}/${MACHINE_CPU}

KMOD=	ptyfs
SRCS=	ptyfs_vfsops.c ptyfs_vnops.c ptyfs_subr.c

.include <bsd.kmodule.mk>

#	$NetBSD: Makefile,v 1.1.2.2 2008/06/29 09:33:17 mjf Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/ptyfs ${S}/ ${S}/arch/${MACHINE}/${MACHINE} \
	${S}/arch/${MACHINE_CPU}/${MACHINE_CPU}

KMOD=	ptyfs
SRCS=	ptyfs_vfsops.c ptyfs_vnops.c ptyfs_subr.c

.include <bsd.kmodule.mk>

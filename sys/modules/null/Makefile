#	$NetBSD: Makefile,v 1.2 2011/05/26 12:56:31 joerg Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/nullfs ${S}/miscfs/genfs

KMOD=	null
SRCS=	null_vfsops.c null_vnops.c

CWARNFLAGS.clang=	-Wno-conversion

.include <bsd.kmodule.mk>

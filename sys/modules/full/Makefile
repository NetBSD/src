#	$NetBSD$

.include "../Makefile.inc"

.PATH:	${S}/miscfs/fullfs ${S}/miscfs/genfs

KMOD=	full
SRCS=	full_vfsops.c full_vnops.c

CWARNFLAGS.clang=	-Wno-conversion

.include <bsd.kmodule.mk>

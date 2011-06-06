#	$NetBSD: Makefile,v 1.1.16.1 2011/06/06 09:09:49 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/nullfs ${S}/miscfs/genfs

KMOD=	null
SRCS=	null_vfsops.c null_vnops.c

CWARNFLAGS.clang=	-Wno-conversion

.include <bsd.kmodule.mk>

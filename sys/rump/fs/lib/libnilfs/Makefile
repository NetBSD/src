#	$NetBSD: Makefile,v 1.2.4.1 2011/06/06 09:10:05 jruoho Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/nilfs

LIB=	rumpfs_nilfs

SRCS=	nilfs_subr.c nilfs_vfsops.c nilfs_vnops.c

CWARNFLAGS.clang+=	-Wno-self-assign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

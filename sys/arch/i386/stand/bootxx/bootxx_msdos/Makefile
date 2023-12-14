# $NetBSD: Makefile,v 1.4.24.1 2023/12/14 17:56:08 martin Exp $

NOMAN=yes
.include <bsd.init.mk>

PROG=	bootxx_msdos
FS=	dosfs
CPPFLAGS=-DBOOT_FROM_FAT -DTERSE_ERROR -DSA_DOSFS_NO_BIG_PART_SUPPORT

.include <../Makefile.bootxx>
OPT_SIZE.clang += -Oz

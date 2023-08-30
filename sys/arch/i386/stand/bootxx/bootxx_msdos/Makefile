# $NetBSD: Makefile,v 1.5 2023/08/30 18:46:51 christos Exp $

NOMAN=yes
.include <bsd.init.mk>

PROG=	bootxx_msdos
FS=	dosfs
CPPFLAGS=-DBOOT_FROM_FAT -DTERSE_ERROR -DSA_DOSFS_NO_BIG_PART_SUPPORT

.include <../Makefile.bootxx>
OPT_SIZE.clang += -Oz

#	$NetBSD: Makefile,v 1.1.2.1 2024/06/29 19:43:26 perseant Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/exfatfs

LIB=	rumpfs_exfatfs
COMMENT=Microsoft exFAT file system

SRCS=	exfatfs_cksum.c exfatfs_conv.c exfatfs_extern.c \
	exfatfs_rename.c exfatfs_tables.c \
	exfatfs_trie_basic.c exfatfs_vfsops.c exfatfs_vnops.c

CPPFLAGS+=

CWARNFLAGS.gcc+=	${GCC_NO_ADDR_OF_PACKED_MEMBER} ${GCC_NO_FORMAT_TRUNCATION}
CWARNFLAGS.gcc+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 9:? -Wno-error=restrict :}

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

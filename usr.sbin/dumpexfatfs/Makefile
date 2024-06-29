#	$NetBSD: Makefile,v 1.1.2.1 2024/06/29 19:43:27 perseant Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

WARNS?=	3	# XXX -Wsign-compare

.include <bsd.own.mk>

CFLAGS+=-g
PROG=	dumpexfatfs
SRCS=	dumpexfatfs.c exfatfs.c exfatfs_cksum.c exfatfs_conv.c exfatfs_extern.c exfatfs_trie_basic.c exfatfs_tables.c bufcache.c vnode.c kernelops.c
.PATH:	${NETBSDSRCDIR}/sys/fs/exfatfs
MAN=	dumpexfatfs.8
LDADD+=	-lutil
DPADD+= ${LIBUTIL}
FSCK_LFS=${NETBSDSRCDIR}/sbin/fsck_lfs
EXFATFS=${NETBSDSRCDIR}/sys/fs/exfatfs
CPPFLAGS+= -I. -I${FSCK_LFS} -DEXFATFS_DEBUG
.PATH: ${FSCK_LFS} ${EXFATFS}

.include <bsd.prog.mk>

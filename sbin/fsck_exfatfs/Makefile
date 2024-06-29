# $NetBSD: Makefile,v 1.1.2.1 2024/06/29 19:43:25 perseant Exp $

.include <bsd.own.mk>

PROG=	fsck_exfatfs
MAN=	fsck_exfatfs.8
SRCS=	fsck_exfatfs.c exfatfs.c partutil.c bufcache.c exfatfs_cksum.c exfatfs_conv.c exfatfs_extern.c exfatfs_tables.c exfatfs_trie.c vnode.c kernelops.c pass0.c pass1.c pass2.c fsutil.c

LDADD+= -lutil
DPADD+= ${LIBUTIL}

LDADD+=-lprop
DPADD+=${LIBPROP}

#LDADD+=-lefence -Wl,-rpath,/usr/pkg/lib -L/usr/pkg/lib

FSCK=${NETBSDSRCDIR}/sbin/fsck
FSCK_LFS=${NETBSDSRCDIR}/sbin/fsck_lfs
DUMPEXFATFS=${NETBSDSRCDIR}/usr.sbin/dumpexfatfs
EXFATFS=${NETBSDSRCDIR}/sys/fs/exfatfs
CPPFLAGS+=-I${.CURDIR} -I${FSCK} -I${FSCK_LFS} -g -O0
.PATH:  ${FSCK} ${FSCK_LFS} ${DUMPEXFATFS} ${EXFATFS}

.include <bsd.prog.mk>

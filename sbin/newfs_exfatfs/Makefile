# $NetBSD: Makefile,v 1.1.2.1 2024/06/29 19:43:25 perseant Exp $

.include <bsd.own.mk>

PROG=	newfs_exfatfs
MAN=	newfs_exfatfs.8
SRCS=	newfs.c partutil.c make_exfatfs.c bufcache.c exfatfs_cksum.c exfatfs_conv.c exfatfs_tables.c vnode.c kernelops.c

CFLAGS=-g

LDADD+= -lutil
DPADD+= ${LIBUTIL}

LDADD+=-lprop
DPADD+=${LIBPROP}

FSCK=${NETBSDSRCDIR}/sbin/fsck
FSCK_LFS=${NETBSDSRCDIR}/sbin/fsck_lfs
EXFATFS=${NETBSDSRCDIR}/sys/fs/exfatfs
CPPFLAGS+=-I${.CURDIR} -I${FSCK} -I${FSCK_LFS}
.PATH:  ${FSCK} ${FSCK_LFS} ${EXFATFS}

.include <bsd.prog.mk>

# $NetBSD: Makefile,v 1.1.2.4 2025/09/06 21:51:10 perseant Exp $

.include <bsd.own.mk>

PROG=	fsck_exfatfs
MAN=	fsck_exfatfs.8
SRCS=	fsck_exfatfs.c exfatfs.c partutil.c bufcache.c exfatfs_balloc.c \
	exfatfs_cksum.c exfatfs_conv.c exfatfs_extern.c exfatfs_tables.c \
	vnode.c kernelops.c pass0.c pass1.c pass2.c pass3.c fsutil.c defaults.c

LDADD+= -lutil
DPADD+= ${LIBUTIL}

LDADD+=-lprop
DPADD+=${LIBPROP}

#LDADD+=-lefence -Wl,-rpath,/usr/pkg/lib -L/usr/pkg/lib

FSCK=${NETBSDSRCDIR}/sbin/fsck
NEWFS_EXFATFS=${NETBSDSRCDIR}/sbin/newfs_exfatfs
DUMPEXFATFS=${NETBSDSRCDIR}/usr.sbin/dumpexfatfs
EXFATFS=${NETBSDSRCDIR}/sys/fs/exfatfs
CPPFLAGS+=-I${.CURDIR} -I${FSCK} -I${NEWFS_EXFATFS} -g -O0
.PATH:  ${FSCK} ${NEWFS_EXFATFS} ${DUMPEXFATFS} ${EXFATFS}

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.8 2024/04/20 13:24:49 rillig Exp $

.include <bsd.own.mk>

PROG=resize_ffs
MAN=resize_ffs.8
SRCS=resize_ffs.c ffs_bswap.c progress.c

CPPFLAGS+=-I${NETBSDSRCDIR}/sbin/fsck

DPADD+= ${LIBUTIL}
LDADD+= -lutil

.PATH: ${NETBSDSRCDIR}/sys/ufs/ffs
.PATH: ${NETBSDSRCDIR}/sbin/fsck

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.7 2021/09/14 20:13:03 rillig Exp $

.include <bsd.own.mk>

PROG=resize_ffs
MAN=resize_ffs.8
SRCS=resize_ffs.c ffs_bswap.c progress.c

CPPFLAGS+=-I${NETBSDSRCDIR}/sbin/fsck

DPADD+= ${LIBUTIL}
LDADD+= -lutil

.PATH: ${NETBSDSRCDIR}/sys/ufs/ffs
.PATH: ${NETBSDSRCDIR}/sbin/fsck

# resize_ffs.c(90): error: cannot take size/alignment of incomplete type [143]
LINT_SUPPORTED=	no

.include <bsd.prog.mk>

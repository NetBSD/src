#	$NetBSD: Makefile,v 1.8 2025/09/18 03:36:01 mrg Exp $

WARNS?=	3	# XXX: sign-compare issues

.include <bsd.own.mk>

PROG=resize_lfs
SRCS=resize_lfs.c partutil.c
MAN=resize_lfs.8

FSCK=${NETBSDSRCDIR}/sbin/fsck
CPPFLAGS+=-I${FSCK}

DPADD+= ${LIBUTIL}
LDADD+= -lutil

LDADD+=-lprop
DPADD+=${LIBPROP}

.PATH: ${FSCK}

COPTS.resize_lfs.c+=	${CC_WNO_CALLOC_TRANSPOSED_ARGS}

.include <bsd.prog.mk>

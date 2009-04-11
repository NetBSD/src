#	$NetBSD: Makefile,v 1.4 2009/04/11 07:58:13 lukem Exp $

WARNS?=	3	# XXX: sign-compare issues

.include <bsd.own.mk>

PROG=resize_lfs
SRCS=resize_lfs.c partutil.c
MAN=resize_lfs.8

FSCK=${NETBSDSRCDIR}/sbin/fsck
CPPFLAGS+=-I${FSCK}
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.PATH: ${FSCK}

.include <bsd.prog.mk>

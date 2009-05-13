#	$NetBSD: Makefile,v 1.3.28.1 2009/05/13 19:19:05 jym Exp $

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

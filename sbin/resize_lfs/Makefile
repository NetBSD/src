#	$NetBSD: Makefile,v 1.3 2006/09/07 23:53:41 riz Exp $

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

#	$NetBSD: Makefile,v 1.2 2006/09/05 19:46:14 riz Exp $

PROG=resize_lfs
SRCS=resize_lfs.c partutil.c
MAN=resize_lfs.8

FSCK=${NETBSDSRCDIR}/sbin/fsck
CPPFLAGS+=-I${FSCK}
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.PATH: ${FSCK}

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.10 1997/06/21 19:16:19 veego Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	quotacheck
SRCS=	quotacheck.c fsutil.c preen.c
MAN=	quotacheck.8
CFLAGS+=-I${.CURDIR}/../../sbin/fsck
.PATH:	${.CURDIR}/../../sbin/fsck

.include <bsd.prog.mk>

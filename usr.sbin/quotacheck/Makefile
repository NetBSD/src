#	$NetBSD: Makefile,v 1.12 1997/10/18 06:42:54 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	quotacheck
SRCS=	quotacheck.c fsutil.c preen.c
MAN=	quotacheck.8
CFLAGS+=-I${.CURDIR}/../../sbin/fsck
.PATH:	${.CURDIR}/../../sbin/fsck

.include <bsd.prog.mk>

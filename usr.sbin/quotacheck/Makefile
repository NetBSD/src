#	$NetBSD: Makefile,v 1.11 1997/09/16 13:07:59 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

WARNS?=	1
PROG=	quotacheck
SRCS=	quotacheck.c fsutil.c preen.c
MAN=	quotacheck.8
CFLAGS+=-I${.CURDIR}/../../sbin/fsck
.PATH:	${.CURDIR}/../../sbin/fsck

.include <bsd.prog.mk>

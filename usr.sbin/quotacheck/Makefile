#	$NetBSD: Makefile,v 1.13 1997/10/25 06:58:34 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	quotacheck
SRCS=	quotacheck.c fsutil.c preen.c
MAN=	quotacheck.8
CPPFLAGS+=-I${.CURDIR}/../../sbin/fsck
.PATH:	${.CURDIR}/../../sbin/fsck

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.12.2.1 1997/11/08 15:12:24 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	quotacheck
SRCS=	quotacheck.c fsutil.c preen.c
MAN=	quotacheck.8
CPPFLAGS+=-I${.CURDIR}/../../sbin/fsck
.PATH:	${.CURDIR}/../../sbin/fsck

.include <bsd.prog.mk>

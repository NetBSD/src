#	$NetBSD: Makefile,v 1.9 1997/05/08 21:11:31 gwr Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	quotacheck
SRCS=	quotacheck.c fsutil.c preen.c
MAN=	quotacheck.8
CFLAGS+=-I${.CURDIR}/../fsck
.PATH:	${.CURDIR}/../fsck

.include <bsd.prog.mk>

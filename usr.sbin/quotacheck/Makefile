#	$NetBSD: Makefile,v 1.7 1996/09/27 23:25:34 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	quotacheck
SRCS=	quotacheck.c fsutil.c preen.c
MAN=	quotacheck.8
CFLAGS+=-I${.CURDIR}/../fsck
.PATH:	${.CURDIR}/../fsck

.include <bsd.prog.mk>

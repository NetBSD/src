#	$NetBSD: Makefile,v 1.8 1997/05/06 20:45:24 gwr Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	quotacheck
SRCS=	quotacheck.c fsutil.c preen.c
MAN=	quotacheck.8
CFLAGS+=-I${.CURDIR}/../fsck
.PATH.c:  ${.CURDIR}/../fsck

.include <bsd.prog.mk>

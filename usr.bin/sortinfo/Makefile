#	$NetBSD: Makefile,v 1.2 2015/12/19 17:30:00 joerg Exp $

WARNS=6
PROG=sortinfo

.ifndef HOSTPROG
LDADD+=-lutil
DPADD+=${LIBUTIL}
.endif

.include <bsd.prog.mk>

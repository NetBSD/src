#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.16 2002/01/29 10:20:38 tv Exp $

PROG=	pwd_mkdb
MAN=	pwd_mkdb.8
CPPFLAGS+=	-I${.CURDIR}/../../lib/libc/include -D__DBINTERFACE_PRIVATE

.ifndef HOSTPROG
LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
.endif

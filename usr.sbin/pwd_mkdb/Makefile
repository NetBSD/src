#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.15 2002/01/15 11:41:43 lukem Exp $

PROG=	pwd_mkdb
MAN=	pwd_mkdb.8
LDADD+=	-lutil
DPADD+=	${LIBUTIL}
CPPFLAGS+=	-I${.CURDIR}/../../lib/libc/include -D__DBINTERFACE_PRIVATE

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif

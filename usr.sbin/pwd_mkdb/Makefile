#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.18 2003/05/18 07:57:40 lukem Exp $

.include <bsd.own.mk>

PROG=	pwd_mkdb
MAN=	pwd_mkdb.8
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/include -D__DBINTERFACE_PRIVATE

.ifndef HOSTPROG
LDADD+=	-lutil
DPADD+=	${LIBUTIL}
.endif

.include <bsd.prog.mk>

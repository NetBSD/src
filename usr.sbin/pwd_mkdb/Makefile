#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.18.40.1 2008/12/28 01:21:23 christos Exp $

.include <bsd.own.mk>

PROG=	pwd_mkdb
MAN=	pwd_mkdb.8
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/include

.ifndef HOSTPROG
LDADD+=	-lutil
DPADD+=	${LIBUTIL}
.endif

.include <bsd.prog.mk>

#	$NetBSD$

.include <bsd.own.mk>

PROG=	fullfsctl
MAN=	fullfsctl.8

CPPFLAGS+= -I${NETBSDSRCDIR}/sys
LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

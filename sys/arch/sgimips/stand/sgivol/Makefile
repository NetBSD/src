#	$NetBSD: Makefile,v 1.5.22.1 2006/06/21 14:55:31 yamt Exp $

NOMAN=	# defined

PROG=	sgivol
.if (${HOSTPROG:U} == "")
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.endif

WARNS=	2

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.6 2005/12/23 03:28:10 ginsbach Exp $

NOMAN=	# defined

PROG=	sgivol
.if (${HOSTPROG:U} == "")
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.endif

WARNS=	2

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.8.142.2 2016/10/05 20:55:24 skrll Exp $
#

NOMAN=	# defined
.include <bsd.init.mk>

PROG=txlt
CLEANFILES+=	txlt.c

.ifndef HOSTPROG
LDADD+=		-ll
DPADD+=		${LIBL}
.endif

.include <bsd.prog.mk>

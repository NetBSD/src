#	$NetBSD: Makefile,v 1.10 2016/09/17 17:09:10 christos Exp $
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

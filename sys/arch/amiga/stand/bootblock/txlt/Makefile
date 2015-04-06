#	$NetBSD: Makefile,v 1.8.142.1 2015/04/06 15:17:51 skrll Exp $
#
PROG=txlt
NOMAN=	# defined
CLEANFILES+=	txlt.c

.ifndef HOSTPROG
LDADD+=		-ll
DPADD+=		${LIBL}
.endif

.include <bsd.prog.mk>

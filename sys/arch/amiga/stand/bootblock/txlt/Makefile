#	$NetBSD: Makefile,v 1.8.122.1 2017/12/03 11:35:49 jdolecek Exp $
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

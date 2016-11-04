#	$NetBSD: Makefile,v 1.9.2.1 2016/11/04 14:48:58 pgoyette Exp $
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

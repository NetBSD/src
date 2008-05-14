#	$NetBSD: Makefile,v 1.2 2008/05/14 20:13:05 reinoud Exp $
#

PROG=		udf

COPTS+=		-DUDF_READWRITE -DDEBUG -DDIAGNOSTIC
RUMPFSLD_UDF=	-Wl,--whole-archive -lrump -Wl,--no-whole-archive

LDADD+=		${RUMPFSLD_UDF}
DPADD+=		${RUMPFSDP_UDF}

.include <bsd.prog.mk>

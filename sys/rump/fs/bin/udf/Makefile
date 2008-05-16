#	$NetBSD: Makefile,v 1.1.34.1 2008/05/16 02:25:50 yamt Exp $
#

PROG=		udf

COPTS+=		-DUDF_READWRITE -DDEBUG -DDIAGNOSTIC
RUMPFSLD_UDF=	-Wl,--whole-archive -lrump -Wl,--no-whole-archive

LDADD+=		${RUMPFSLD_UDF}
DPADD+=		${RUMPFSDP_UDF}

.include <bsd.prog.mk>

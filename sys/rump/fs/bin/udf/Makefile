#	$NetBSD: Makefile,v 1.1.32.1 2008/05/18 12:35:45 yamt Exp $
#

PROG=		udf

COPTS+=		-DUDF_READWRITE -DDEBUG -DDIAGNOSTIC
RUMPFSLD_UDF=	-Wl,--whole-archive -lrump -Wl,--no-whole-archive

LDADD+=		${RUMPFSLD_UDF}
DPADD+=		${RUMPFSDP_UDF}

.include <bsd.prog.mk>

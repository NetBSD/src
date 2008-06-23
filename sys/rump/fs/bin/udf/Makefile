#	$NetBSD: Makefile,v 1.1.36.1 2008/06/23 04:32:02 wrstuden Exp $
#

PROG=		udf

COPTS+=		-DUDF_READWRITE -DDEBUG -DDIAGNOSTIC
RUMPFSLD_UDF=	-Wl,--whole-archive -lrump -Wl,--no-whole-archive

LDADD+=		${RUMPFSLD_UDF}
DPADD+=		${RUMPFSDP_UDF}

.include <bsd.prog.mk>

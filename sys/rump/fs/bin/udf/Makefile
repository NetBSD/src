#	$NetBSD: Makefile,v 1.1.30.1 2008/06/02 13:24:31 mjf Exp $
#

PROG=		udf

COPTS+=		-DUDF_READWRITE -DDEBUG -DDIAGNOSTIC
RUMPFSLD_UDF=	-Wl,--whole-archive -lrump -Wl,--no-whole-archive

LDADD+=		${RUMPFSLD_UDF}
DPADD+=		${RUMPFSDP_UDF}

.include <bsd.prog.mk>

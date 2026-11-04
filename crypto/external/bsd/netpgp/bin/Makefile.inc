#	$NetBSD: Makefile.inc,v 1.1.4.3 2026/11/04 22:36:35 perseant Exp $

EXTDIST:=        ${.PARSEDIR}/../dist

CPPFLAGS+=	-I${EXTDIST}/include

LIBNETPGPDIR!=	cd ${.PARSEDIR}/../lib/netpgp && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETPGPDIR} -lnetpgp
DPADD+=		${LIBNETPGPDIR}/libnetpgp.a

LIBMJDIR!=	cd ${.PARSEDIR}/../libmj && ${PRINTOBJDIR}
LDADD+=		-L${LIBMJDIR} -lmj
DPADD+=		${LIBMJDIR}/libmj.a

LDADD+=		-lcrypto -lz -lbz2
DPADD+=		${LIBCRYPTO} ${LIBZ} ${LIBBZ2}

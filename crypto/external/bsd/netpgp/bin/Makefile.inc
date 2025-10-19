#	$NetBSD: Makefile.inc,v 1.2 2025/10/19 00:59:58 riastradh Exp $

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

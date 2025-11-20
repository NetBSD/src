#	$NetBSD: Makefile.inc,v 1.1.2.1 2025/11/20 18:46:45 martin Exp $

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

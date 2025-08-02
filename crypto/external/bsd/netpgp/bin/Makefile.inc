#	$NetBSD: Makefile.inc,v 1.1.4.2 2025/08/02 05:18:42 perseant Exp $

EXTDIST:=        ${.PARSEDIR}/../dist

CPPFLAGS+=	-I${EXTDIST}/include

LIBNETPGPDIR!=	cd ${.PARSEDIR}/../lib/netpgp && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETPGPDIR} -lnetpgp
DPADD+=		${LIBNETPGPDIR}/libnetpgp.a

LIBMJDIR!=	cd ${.PARSEDIR}/../libmj && ${PRINTOBJDIR}
LDADD+=		-L${LIBMJDIR} -lmj
DPADD+=		${LIBMJDIR}/libmj.a

LDADD+=		-lcrypto -lz -lbz2 -lpthread
DPADD+=		${LIBCRYPTO} ${LIBZ} ${LIBBZ2} ${LIBPTHREAD}

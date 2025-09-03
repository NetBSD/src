#	$NetBSD: Makefile.inc,v 1.1 2025/07/19 11:26:23 christos Exp $

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

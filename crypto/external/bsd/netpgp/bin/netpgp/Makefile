#	$NetBSD: Makefile,v 1.2.8.2 2014/08/19 23:45:22 tls Exp $

.include <bsd.own.mk>

PROG=		netpgp
BINDIR=		/usr/bin

CPPFLAGS+=	-I${EXTDIST}/include

LIBNETPGPDIR!=	cd ${.CURDIR}/../../lib && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETPGPDIR} -lnetpgp
DPADD+=		${LIBNETPGPDIR}/libnetpgp.a

LIBMJDIR!=	cd ${.CURDIR}/../../libmj && ${PRINTOBJDIR}
LDADD+=		-L${LIBMJDIR} -lmj
DPADD+=		${LIBMJDIR}/libmj.a

LDADD+=		-lcrypto -lz -lbz2
DPADD+=		${LIBCRYPTO} ${LIBZ} ${LIBBZ2}

MAN=		netpgp.1

EXTDIST=	${.CURDIR}/../../dist
.PATH: ${EXTDIST}/src/netpgp

.include <bsd.prog.mk>

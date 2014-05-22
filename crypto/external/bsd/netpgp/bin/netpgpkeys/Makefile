#	$NetBSD: Makefile,v 1.2.4.2 2014/05/22 13:21:32 yamt Exp $

.include <bsd.own.mk>

PROG=		netpgpkeys
SRCS=		netpgpkeys.c
BINDIR=		/usr/bin

LIBNETPGPDIR!=	cd ${.CURDIR}/../../lib && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETPGPDIR} -lnetpgp
DPADD+=		${LIBNETPGPDIR}/libnetpgp.a

LIBMJDIR!=	cd ${.CURDIR}/../../libmj && ${PRINTOBJDIR}
LDADD+=		-L${LIBMJDIR} -lmj
DPADD+=		${LIBMJDIR}/libmj.a

LDADD+=		-lcrypto -lz -lbz2
DPADD+=		${LIBCRYPTO} ${LIBZ} ${LIBBZ2}

MAN=		netpgpkeys.1

.PATH: ${.CURDIR}/../../dist/src/netpgpkeys

.include <bsd.prog.mk>

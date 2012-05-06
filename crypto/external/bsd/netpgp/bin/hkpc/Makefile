# $NetBSD: Makefile,v 1.1.2.1 2012/05/06 17:57:11 agc Exp $

.include <bsd.own.mk>

PROG=hkpc
BINDIR=/usr/bin
SRCS=hkpc.c main.c

LIBNETPGPDIR!=	cd ${.CURDIR}/../../lib/netpgp && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETPGPDIR} -lnetpgp
DPADD+=		${LIBNETPGPDIR}/libnetpgp.a

LIBMJDIR!=	cd ${.CURDIR}/../../lib/mj && ${PRINTOBJDIR}
LDADD+=		-L${LIBMJDIR} -lmj
DPADD+=		${LIBMJDIR}/libmj.a

LDADD+=		-lz -lbz2
DPADD+=		${LIBZ} ${LIBBZ2}

WARNS=0
MAN=hkpc.1

EXTDIST=	${.CURDIR}/../../dist/src/hkpclient
.PATH:		${EXTDIST}/src/hkpclient

.include <bsd.prog.mk>

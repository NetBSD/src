# $NetBSD: Makefile,v 1.2.8.2 2014/08/19 23:45:22 tls Exp $

.include <bsd.own.mk>

PROG=hkpd
BINDIR=/usr/sbin
SRCS=hkpd.c main.c

LIBNETPGPDIR!=	cd ${.CURDIR}/../../lib && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETPGPDIR} -lnetpgp
DPADD+=		${LIBNETPGPDIR}/libnetpgp.a

LIBMJDIR!=	cd ${.CURDIR}/../../libmj && ${PRINTOBJDIR}
LDADD+=		-L${LIBMJDIR} -lmj
DPADD+=		${LIBMJDIR}/libmj.a

LDADD+=		-lcrypto -lz -lbz2
DPADD+=		${LIBCRYPTO} ${LIBZ} ${LIBBZ2}

MAN=hkpd.1

.PATH: ${.CURDIR}/../../dist/src/hkpd

.include <bsd.prog.mk>

t: ${PROG}
	./${PROG} -D &
	sleep 1
	ftp -o- 'http://localhost:11371/pks/lookup?op=index&search=agc&options=json'
	ftp -o- 'http://localhost:11371/pks/lookup?op=get&search=agc&options=json'
	ftp -o- 'http://localhost:11371/pks/lookup?op=get&search=agc&options=mr'
	pkill hkpd

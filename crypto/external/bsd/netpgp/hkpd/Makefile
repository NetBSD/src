# $NetBSD: Makefile,v 1.1 2010/09/02 05:58:00 agc Exp $

.include <bsd.own.mk>

PROG=hkpd
BINDIR=/usr/sbin
SRCS=hkpd.c main.c

LIBNETPGPDIR!=	cd ${.CURDIR}/../lib && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETPGPDIR} -lnetpgp
DPADD+=		${LIBNETPGPDIR}/libnetpgp.a

LIBMJDIR!=	cd ${.CURDIR}/../libmj && ${PRINTOBJDIR}
LDADD+=		-L${LIBMJDIR} -lmj
DPADD+=		${LIBMJDIR}/libmj.a

LDADD+=		-lcrypto -lz -lbz2
DPADD+=		${LIBCRYPTO} ${LIBZ} ${LIBBZ2}

MAN=hkpd.1
WARNS=0 # anything over 0 will fail at the link stage with IDEA errors

.PATH: ${.CURDIR}/../dist/src/hkpd

.include <bsd.prog.mk>

t: ${PROG}
	./${PROG} -D &
	sleep 1
	ftp -o- 'http://localhost:11371/pks/lookup?op=index&search=agc&options=json'
	ftp -o- 'http://localhost:11371/pks/lookup?op=get&search=agc&options=json'
	ftp -o- 'http://localhost:11371/pks/lookup?op=get&search=agc&options=mr'
	pkill hkpd

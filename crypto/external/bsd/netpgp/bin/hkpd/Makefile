# $NetBSD: Makefile,v 1.1.2.1 2012/05/06 17:57:11 agc Exp $

.include <bsd.own.mk>

PROG=hkpd
BINDIR=/usr/sbin
SRCS=hkpd.c main.c

LIBNETPGPDIR!=	cd ${.CURDIR}/../../lib/netpgp && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETPGPDIR} -lnetpgp
DPADD+=		${LIBNETPGPDIR}/libnetpgp.a

LIBMJDIR!=	cd ${.CURDIR}/../../lib/mj && ${PRINTOBJDIR}
LDADD+=		-L${LIBMJDIR} -lmj
DPADD+=		${LIBMJDIR}/libmj.a

LIBNETRSADIR!=	cd ${.CURDIR}/../../lib/rsa && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETRSADIR} -lnetpgprsa
DPADD+=		${LIBNETRSADIR}/libnetpgprsa.a

LIBNETBNDIR!=	cd ${.CURDIR}/../../lib/bn && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETBNDIR} -lnetpgpbn
DPADD+=		${LIBNETBNDIR}/libnetpgpbn.a

LIBNETCIPHERDIR!=	cd ${.CURDIR}/../../lib/cipher && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETCIPHERDIR} -lnetpgpcipher
DPADD+=		${LIBNETCIPHERDIR}/libnetpgpcipher.a

LIBNETPGPVERIFYDIR!=	cd ${.CURDIR}/../../lib/verify && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETPGPVERIFYDIR} -lnetpgpverify
DPADD+=		${LIBNETPGPVERIFYDIR}/libnetpgpverify.a

LDADD+=		-lz -lbz2
DPADD+=		${LIBZ} ${LIBBZ2}

MAN=hkpd.1
WARNS=0 # anything over 0 will fail at the link stage with IDEA errors

EXTDIST=	${.CURDIR}/../dist/src/hkpd
.PATH:		${EXTDIST}/src/hkpd

.include <bsd.prog.mk>

t: ${PROG}
	./${PROG} -D &
	sleep 1
	ftp -o- 'http://localhost:11371/pks/lookup?op=index&search=agc&options=json'
	ftp -o- 'http://localhost:11371/pks/lookup?op=get&search=agc&options=json'
	ftp -o- 'http://localhost:11371/pks/lookup?op=get&search=agc&options=mr'
	pkill hkpd

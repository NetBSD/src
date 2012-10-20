#	$NetBSD: Makefile,v 1.1.2.2 2012/10/20 04:59:52 agc Exp $

.include <bsd.own.mk>

PROG=		pgp2ssh
BINDIR=		/usr/bin

SRCS=		b64.c hkpc.c main.c
CPPFLAGS+=	-I${.CURDIR}/../dist/include -I${.CURDIR}/../dist/src/hkpclient

LIBNETPGPDIR!=	cd ${.CURDIR}/../../lib/netpgp && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETPGPDIR} -lnetpgp
DPADD+=		${LIBNETPGPDIR}/libnetpgp.a

LIBNETPGPVERIFYDIR!=	cd ${.CURDIR}/../../lib/verify && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETPGPVERIFYDIR} -lnetpgpverify
DPADD+=		${LIBNETPGPVERIFYDIR}/libnetpgpverify.a

LIBNETCIPHERDIR!=	cd ${.CURDIR}/../../lib/cipher && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETCIPHERDIR} -lnetpgpcipher
DPADD+=		${LIBNETCIPHERDIR}/libnetpgpcipher.a

LIBNETRSADIR!=	cd ${.CURDIR}/../../lib/rsa && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETRSADIR} -lnetpgprsa
DPADD+=		${LIBNETRSADIR}/libnetpgprsa.a

LIBNETDIGESTDIR!=	cd ${.CURDIR}/../../lib/digest && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETDIGESTDIR} -lnetpgpdigest
DPADD+=		${LIBNETDIGESTDIR}/libnetpgpdigest.a

LIBNETBNDIR!=	cd ${.CURDIR}/../../lib/bn && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETBNDIR} -lnetpgpbn
DPADD+=		${LIBNETBNDIR}/libnetpgpbn.a

LIBMJDIR!=	cd ${.CURDIR}/../../lib/mj && ${PRINTOBJDIR}
LDADD+=		-L${LIBMJDIR} -lmj
DPADD+=		${LIBMJDIR}/libmj.a

LDADD+=		-lz -lbz2
DPADD+=		${LIBZ} ${LIBBZ2}

MAN=		pgp2ssh.1
WARNS=		0

EXTDIST=	${.CURDIR}/../../dist
.PATH:		${EXTDIST}/src/pgp2ssh ${EXTDIST}/src/hkpclient

.include <bsd.prog.mk>

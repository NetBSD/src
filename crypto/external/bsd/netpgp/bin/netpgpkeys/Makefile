#	$NetBSD: Makefile,v 1.1.2.2 2012/10/20 04:38:44 agc Exp $

.include <bsd.own.mk>

PROG=		netpgpkeys
SRCS=		netpgpkeys.c
BINDIR=		/usr/bin

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

MAN=		netpgpkeys.1

WARNS=		4

EXTDIST=	${.CURDIR}/../../dist
.PATH:		${EXTDIST}/src/netpgpkeys

.include <bsd.prog.mk>

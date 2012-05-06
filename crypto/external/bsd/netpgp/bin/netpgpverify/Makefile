#	$NetBSD: Makefile,v 1.1.2.1 2012/05/06 17:57:11 agc Exp $

.include <bsd.own.mk>

PROG=		netpgpverify
BINDIR=		/usr/bin
SRCS=		verify.c

CPPFLAGS+=	-I${EXTDIST}/include -I${.CURDIR}/../../lib/netpgp
CPPFLAGS+=	-I${EXTDIST}/src/libbn
CPPFLAGS+=	-I${EXTDIST}/src/librsa

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

LIBNETBNDIR!=	cd ${.CURDIR}/../../lib/bn && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETBNDIR} -lnetpgpbn
DPADD+=		${LIBNETBNDIR}/libnetpgpbn.a

LIBMJDIR!=	cd ${.CURDIR}/../../lib/mj && ${PRINTOBJDIR}
LDADD+=		-L${LIBMJDIR} -lmj
DPADD+=		${LIBMJDIR}/libmj.a

LDADD+=		-lz -lbz2
DPADD+=		${LIBZ} ${LIBBZ2}

MAN=		netpgpverify.1

WARNS=		4

EXTDIST=	${.CURDIR}/../../dist
.PATH:		${EXTDIST}/src/netpgpverify ${EXTDIST}/src/libnetpgp

.include <bsd.prog.mk>

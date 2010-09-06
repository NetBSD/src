#	$NetBSD: Makefile,v 1.1 2010/09/06 18:17:58 agc Exp $

.include <bsd.own.mk>

PROG=		pgp2ssh
BINDIR=		/usr/bin

SRCS=		b64.c hkpc.c main.c
CPPFLAGS+=	-I${.CURDIR}/../dist/include -I${.CURDIR}/../dist/src/hkpclient

LIBNETPGPDIR!=	cd ${.CURDIR}/../lib && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETPGPDIR} -lnetpgp
DPADD+=		${LIBNETPGPDIR}/libnetpgp.a

LIBMJDIR!=	cd ${.CURDIR}/../libmj && ${PRINTOBJDIR}
LDADD+=		-L${LIBMJDIR} -lmj
DPADD+=		${LIBMJDIR}/libmj.a

LDADD+=		-lcrypto -lz -lbz2
DPADD+=		${LIBCRYPTO} ${LIBZ} ${LIBBZ2}

MAN=		pgp2ssh.1
WARNS=		0

.PATH: ${.CURDIR}/../dist/src/pgp2ssh ${.CURDIR}/../dist/src/hkpclient

.include <bsd.prog.mk>

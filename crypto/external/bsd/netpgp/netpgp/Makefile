#	$NetBSD: Makefile,v 1.3 2010/09/01 06:02:58 agc Exp $

.include <bsd.own.mk>

PROG=		netpgp
BINDIR=		/usr/bin

CPPFLAGS+=	-I${.CURDIR}/../dist/include

LIBNETPGPDIR!=	cd ${.CURDIR}/../lib && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETPGPDIR} -lnetpgp
DPADD+=		${LIBNETPGPDIR}/libnetpgp.a

LIBMJDIR!=	cd ${.CURDIR}/../libmj && ${PRINTOBJDIR}
LDADD+=		-L${LIBMJDIR} -lmj
DPADD+=		${LIBMJDIR}/libmj.a

LDADD+=		-lcrypto -lz -lbz2
DPADD+=		${LIBCRYPTO} ${LIBZ} ${LIBBZ2}

MAN=		netpgp.1

# although the code is/was WARNS=4 clean, when linking, there is a warning
# about libidea being a patented algorithm, and WARNS>0 treats warnings as
# errors. For now, just set WARNS off.
WARNS=		0

.PATH: ${.CURDIR}/../dist/src/netpgp

.include <bsd.prog.mk>

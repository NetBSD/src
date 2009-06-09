#	$NetBSD: Makefile,v 1.2 2009/06/09 00:56:15 agc Exp $

.include <bsd.own.mk>

PROG=		netpgp
BINDIR=		/usr/bin

CPPFLAGS+=	-I${.CURDIR}/../dist/include

LIBNETPGPDIR!=	cd ${.CURDIR}/../lib && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETPGPDIR} -lnetpgp
DPADD+=		${LIBNETPGPDIR}/libnetpgp.a

LDADD+=		-lcrypto -lz -lbz2
DPADD+=		${LIBCRYPTO} ${LIBZ} ${LIBBZ2}

MAN=		netpgp.1

# although the code is/was WARNS=4 clean, when linking, there is a warning
# about libidea being a patented algorithm, and WARNS>0 treats warnings as
# errors. For now, just set WARNS off.
WARNS=		0

.PATH: ${.CURDIR}/../dist/src/netpgp

.include <bsd.prog.mk>

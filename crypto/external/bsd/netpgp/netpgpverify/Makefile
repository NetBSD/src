#	$NetBSD: Makefile,v 1.3 2009/06/09 00:56:15 agc Exp $

PROG=		netpgpverify
BINDIR=		/usr/bin
SRCS=		verify.c

CPPFLAGS+=	-I${.CURDIR}/../dist/include -I${.CURDIR}/../lib

LDADD+=		-lcrypto -lz -lbz2
DPADD+=		${LIBCRYPTO} ${LIBZ} ${LIBBZ2}

MAN=		netpgpverify.1

# although the code is/was WARNS=4 clean, when linking, there is a warning
# about libidea being a patented algorithm, and WARNS>0 treats warnings as
# errors. For now, just set WARNS off.
WARNS=		0

EXTDIST=${.CURDIR}/../dist

.PATH: ${EXTDIST}/src/netpgpverify

.include <bsd.prog.mk>

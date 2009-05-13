#	$NetBSD: Makefile,v 1.1.1.1.2.2 2009/05/13 19:16:01 jym Exp $

.include <bsd.own.mk>

PROG=		netpgp
BINDIR=		/usr/bin

CPPFLAGS+=	-I${.CURDIR}/../dist/include

LIBNETPGPDIR!=	cd ${.CURDIR}/../lib && ${PRINTOBJDIR}
LDADD+=		-L${LIBNETPGPDIR} -lnetpgp
DPADD+=		${LIBNETPGPDIR}/libnetpgp.a

MAN=		netpgp.1

# although the code is/was WARNS=4 clean, when linking, there is a warning
# about libidea being a patented algorithm, and WARNS>0 treats warnings as
# errors. For now, just set WARNS off.
WARNS=		0

.PATH: ${.CURDIR}/../dist/src/bin

.include <bsd.prog.mk>

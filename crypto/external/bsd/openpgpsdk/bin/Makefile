#	$NetBSD: Makefile,v 1.5 2009/01/30 04:14:19 agc Exp $

.include <bsd.own.mk>

PROG=		openpgp
BINDIR=		/usr/bin

CPPFLAGS+=	-I${.CURDIR}/../dist/include

LIBOPSOBJDIR!=	cd ${.CURDIR}/../lib && ${PRINTOBJDIR}
LDADD+=		-L${LIBOPSOBJDIR} -lopenpgpsdk
DPADD+=		${LIBOPSOBJDIR}/libopenpgpsdk.a

MAN=		openpgp.1

# although the code is/was WARNS=4 clean, when linking, there is a warning
# about libidea being a patented algorithm, and WARNS>0 treats warnings as
# errors. For now, just set WARNS off.
WARNS=		0

.PATH: ${.CURDIR}/../dist/src/app

.include <bsd.prog.mk>

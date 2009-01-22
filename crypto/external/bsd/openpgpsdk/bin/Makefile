#	$NetBSD: Makefile,v 1.4 2009/01/22 00:22:20 lukem Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=		openpgp
BINDIR=		/usr/bin

CPPFLAGS+=	-I${.CURDIR}/../dist/include

LIBOPSOBJDIR!=	cd ${.CURDIR}/../lib && ${PRINTOBJDIR}
LDADD+=		-L${LIBOPSOBJDIR} -lopenpgpsdk
DPADD+=		${LIBOPSOBJDIR}/libopenpgpsdk.a

WARNS=		0

.PATH: ${.CURDIR}/../dist/src/app

.include <bsd.prog.mk>

# $NetBSD: Makefile,v 1.1.2.3 2012/10/30 19:00:45 yamt Exp $
#
# Public Domain
#

PROG=		npftest

SRCS=		npftest.c npfstream.c
CPPFLAGS+=	-I${.CURDIR}

LIBNPFTEST!=	cd ${.CURDIR}/libnpftest && ${MAKE} -V .OBJDIR
DPADD+=		${LIBNPFTEST}/libnpftest.a
LDADD+=		-L${LIBNPFTEST} -lnpftest

LDADD+=		-lrump -lrumpvfs -lrumpnet -lrumpnet_net -lrumpnet_npf

LDADD+=		-lpcap -lprop -lpthread

WARNS=		4
NOMAN=		# no man page
NOLINT=		# disabled (note: deliberately)

SUBDIR+=	libnpftest

${LIBNPFTEST}/libnpftest.a: all-libnpftest

.include <bsd.subdir.mk>
.include <bsd.prog.mk>

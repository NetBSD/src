# $NetBSD: Makefile,v 1.1 2012/04/14 21:57:29 rmind Exp $
#
# Public Domain
#

PROG=		npftest

SRCS=		npftest.c
CPPFLAGS+=	-I${.CURDIR}

LIBNPFTEST!=	cd ${.CURDIR}/libnpftest && ${MAKE} -V .OBJDIR
DPADD+=		${LIBNPFTEST}/libnpftest.a
LDADD+=		-L${LIBNPFTEST} -lnpftest

LDADD+=		-lrump -lrumpvfs -lrumpnet -lrump -lrumpnet_net
LDADD+=		-lrumpnet_virtif -lrumpdev_npf -lpthread

WARNS=		4
NOMAN=		# no man page
NOLINT=		# disabled (note: deliberately)

SUBDIR+=	libnpftest

${LIBNPFTEST}/libnpftest.a: all-libnpftest

.include <bsd.subdir.mk>
.include <bsd.prog.mk>

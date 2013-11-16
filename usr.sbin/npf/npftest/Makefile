# $NetBSD: Makefile,v 1.6 2013/11/16 15:58:30 alnsn Exp $
#
# Public Domain
#

PROG=		npftest

SRCS=		npftest.c npfstream.c
CPPFLAGS+=	-I${.CURDIR}

LIBNPFTEST!=	cd ${.CURDIR}/libnpftest && ${MAKE} -V .OBJDIR
DPADD+=		${LIBNPFTEST}/libnpftest.a
LDADD+=		-L${LIBNPFTEST} -lnpftest

LDADD+=		-lrump -lrumpvfs -lrumpnet -lrumpnet_net
LDADD+=		-lrumpdev_bpf

.if ${MKSLJIT} != "no"
LDADD+=		-lrumpkern_sljit -lrumpnet_bpfjit
.endif

LDADD+=		-lrumpnet_npf

LDADD+=		-lpcap -lprop -lpthread

WARNS=		5
NOMAN=		# no man page
NOLINT=		# disabled (note: deliberately)

SUBDIR+=	libnpftest

${LIBNPFTEST}/libnpftest.a: all-libnpftest

.include <bsd.subdir.mk>
.include <bsd.prog.mk>

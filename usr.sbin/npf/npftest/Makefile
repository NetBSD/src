# $NetBSD$
#
# Public Domain
#

PROG=		npftest
NOMAN=		# no man page

WARNS=		5
NOLINT=		# disabled (note: deliberately)

SRCS=		npftest.c npfstream.c
CPPFLAGS+=	-I${.CURDIR}

LIBNPFTEST!=	cd ${.CURDIR}/libnpftest && ${MAKE} -V .OBJDIR
DPADD+=		${LIBNPFTEST}/libnpftest.a
LDADD+=		-L${LIBNPFTEST} -lnpftest

LDADD+=		-lrump -lrumpvfs -lrumpnet -lrumpnet_net
LDADD+=		-lrumpdev_bpf

.include <bsd.own.mk>

.if ${MKSLJIT} != "no"
LDADD+=		-lrumpkern_sljit -lrumpnet_bpfjit
.endif

LDADD+=		-lrumpnet_npf

LDADD+=		-lpcap -lprop -lpthread

SUBDIR+=	libnpftest

${LIBNPFTEST}/libnpftest.a: all-libnpftest

.include <bsd.subdir.mk>
.include <bsd.prog.mk>

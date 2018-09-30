# $NetBSD: Makefile,v 1.10.12.1 2018/09/30 01:46:01 pgoyette Exp $
#
# Public Domain
#

PROG=		npftest
NOMAN=		# no man page
PAXCTL_FLAGS.npftest += +m

WARNS=		5
NOLINT=		# disabled (note: deliberately)

SRCS=		npftest.c npfstream.c
CPPFLAGS+=	-I${.CURDIR}

LIBNPFTEST!=	cd ${.CURDIR}/libnpftest && ${MAKE} -V .OBJDIR
DPADD+=		${LIBNPFTEST}/libnpftest.a
LDADD+=		-L${LIBNPFTEST} -lnpftest

LDADD+=		-lrump -lrumpvfs -lrumpuser -lrumpnet -lrumpnet_net
LDADD+=		-lrumpdev_bpf
LDADD+=		-lrumpdev

.include <bsd.own.mk>

.if ${MKSLJIT} != "no"
LDADD+=		-lrumpkern_sljit -lrumpnet_bpfjit
.endif

LDADD+=		-lrumpkern_nv -lrumpnet_npf
LDADD+=		-lpcap -lpthread

PROGDPLIBS+=	nv ${NETBSDSRCDIR}/external/bsd/libnv/lib
CPPFLAGS+=	-I ${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

SUBDIR+=	libnpftest

${LIBNPFTEST}/libnpftest.a: all-libnpftest

.include <bsd.subdir.mk>
.include <bsd.prog.mk>

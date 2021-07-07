# $NetBSD: Makefile,v 1.14 2021/07/07 11:51:46 martin Exp $
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

LDADD+=		-lrump -lrumpvfs_nofifofs -lrumpvfs -lrumpuser
LDADD+=		-lrumpnet -lrumpnet_net	-lrumpdev_bpf

.if ${RUMP_SANITIZE:Uno} != "no"
LDADD+=	-fsanitize=${RUMP_SANITIZE}
.endif

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

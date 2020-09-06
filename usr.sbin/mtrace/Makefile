#	$NetBSD: Makefile,v 1.12 2020/09/06 07:20:32 mrg Exp $
#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp

.include <bsd.own.mk>

USE_FORT?= yes	# network client *and* setuid

.PATH: ${NETBSDSRCDIR}/usr.sbin/mrouted
CPPFLAGS+= -I${NETBSDSRCDIR}/usr.sbin/mrouted

PROG=	mtrace
SRCS=	igmp.c inet.c kern.c mtrace.c
MAN=	mtrace.8

.if ${MACHINE_ARCH} == "vax"
COPTS.mtrace.c=-O0
.endif

BINMODE=4555
BINOWN= root

CWARNFLAGS.gcc+=        ${GCC_NO_ADDR_OF_PACKED_MEMBER}

.include <bsd.prog.mk>

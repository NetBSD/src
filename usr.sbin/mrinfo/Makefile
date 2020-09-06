#	$NetBSD: Makefile,v 1.11 2020/09/06 07:20:32 mrg Exp $
#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp

.include <bsd.own.mk>

USE_FORT?= yes	# network client *and* setuid

.PATH: ${NETBSDSRCDIR}/usr.sbin/mrouted
CPPFLAGS+= -I${NETBSDSRCDIR}/usr.sbin/mrouted

PROG=	mrinfo
SRCS=	igmp.c inet.c kern.c mrinfo.c
MAN=	mrinfo.8
BINOWN=	root
BINMODE=4555

CWARNFLAGS.gcc+=        ${GCC_NO_ADDR_OF_PACKED_MEMBER}

.include <bsd.prog.mk>

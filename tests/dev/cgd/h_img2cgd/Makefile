#	$NetBSD: Makefile,v 1.1 2010/11/11 22:38:47 pooka Exp $
#

PROG=	h_img2cgd
SRCS=	img2cgd.c
NOMAN=

.include <bsd.own.mk>

RUMP_ACTION=	#defined
CPPFLAGS+=	-DCGDCONFIG_AS_LIB
.include "${NETBSDSRCDIR}/sbin/cgdconfig/Makefile.cgdconfig"

DBG=	-g -O0
WARNS=	3

.include <bsd.prog.mk>

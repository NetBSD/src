#	$NetBSD: Makefile,v 1.2 2010/11/11 22:44:50 pooka Exp $
#

PROG=		h_img2cgd
SRCS=		img2cgd.c
FILES=		cgd.conf
NOMAN=1

BINDIR=		${TESTSBASE}/dev/cgd/h_img2cgd
FILESDIR=	${TESTSBASE}/dev/cgd/h_img2cgd

.include <bsd.own.mk>

RUMP_ACTION=	#defined
CPPFLAGS+=	-DCGDCONFIG_AS_LIB
.include "${NETBSDSRCDIR}/sbin/cgdconfig/Makefile.cgdconfig"

WARNS=	3

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.1.2.2 2012/10/30 17:22:41 yamt Exp $
#

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	bpfjit
SRCS=	bpfjit.c

CPPFLAGS+=	\
	-I ${NETBSDSRCDIR}/sys/external/bsd/sljit/dist/sljit_src

.include <bsd.kmodule.mk>

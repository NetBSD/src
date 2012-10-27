#	$NetBSD: Makefile,v 1.1.4.2 2012/10/27 22:36:15 alnsn Exp $
#

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	bpfjit
SRCS=	bpfjit.c

CPPFLAGS+=	\
	-I ${NETBSDSRCDIR}/sys/external/bsd/sljit/dist/sljit_src

.include <bsd.kmodule.mk>

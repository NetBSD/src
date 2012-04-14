# $NetBSD: Makefile,v 1.1 2012/04/14 21:57:29 rmind Exp $
#
# Public Domain
#

RUMPTOP=	${.CURDIR}/../../../../sys/rump

LIB=		npftest
LIBISPRIVATE=	yes

SRCS+=		npf_mbuf_subr.c

SRCS+=		npf_nbuf_test.c
SRCS+=		npf_processor_test.c
SRCS+=		npf_table_test.c

CPPFLAGS+=	-I${.CURDIR}/../../../../sys/net/npf
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern

WARNS=		4

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>

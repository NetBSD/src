# $NetBSD: Makefile,v 1.2 2006/02/16 19:19:49 tnozaki Exp $
# FreeBSD: src/usr.bin/locale/Makefile,v 1.4 2003/06/25 23:05:11 phantom Exp

PROG = locale
WARNS ?= 1

CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/locale
CITRUS?=	yes
.if ${CITRUS} == "yes"
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/citrus -DCITRUS
.endif

.include <bsd.prog.mk>

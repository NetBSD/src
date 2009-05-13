# $NetBSD: Makefile,v 1.2.30.1 2009/05/13 19:19:55 jym Exp $
# FreeBSD: src/usr.bin/locale/Makefile,v 1.4 2003/06/25 23:05:11 phantom Exp

WARNS?=	2	# XXX -Wcast-qual issues

PROG = locale

CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/locale
CITRUS?=	yes
.if ${CITRUS} == "yes"
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/citrus -DCITRUS
.endif

.include <bsd.prog.mk>

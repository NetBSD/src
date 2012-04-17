# $NetBSD: Makefile,v 1.3.6.1 2012/04/17 00:09:34 yamt Exp $
# FreeBSD: src/usr.bin/locale/Makefile,v 1.4 2003/06/25 23:05:11 phantom Exp

WARNS?=	2	# XXX -Wcast-qual issues

PROG = locale

CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/locale
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/citrus

.include <bsd.prog.mk>

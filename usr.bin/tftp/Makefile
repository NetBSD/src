#	$NetBSD: Makefile,v 1.6 1999/07/12 20:19:20 itojun Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	tftp
SRCS=	main.c tftp.c tftpsubs.c

# cope with past spec changes
CPPFLAGS+=-Dss_len=__ss_len -Dss_family=__ss_family

.include <bsd.prog.mk>

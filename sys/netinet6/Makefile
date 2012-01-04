#	$NetBSD: Makefile,v 1.7 2012/01/04 15:55:36 drochner Exp $

INCSDIR= /usr/include/netinet6

INCS=	in6.h in6_gif.h in6_ifattach.h in6_pcb.h \
	in6_var.h ip6_mroute.h ip6_var.h ip6protosw.h \
	mld6_var.h nd6.h pim6.h pim6_var.h \
	raw_ip6.h udp6.h udp6_var.h
INCS+=	ipsec.h
# XXX ah.h is used by ping6, rest unneeded
INCS+=	ah.h esp.h ipcomp.h

.include <bsd.kinc.mk>

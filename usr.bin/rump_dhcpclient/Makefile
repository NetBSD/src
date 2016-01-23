#	$NetBSD: Makefile,v 1.2 2016/01/23 21:22:50 christos Exp $
#

PROG=	rump.dhcpclient
SRCS=	main.c bpf.c common.c configure.c dhcp.c if-bsd.c net.c
CPPFLAGS+=	-D_KERNTYPES

LDADD+=	-lrumpclient

.include <bsd.prog.mk>

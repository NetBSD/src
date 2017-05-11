#	$NetBSD: Makefile,v 1.3 2017/05/11 13:20:02 sevan Exp $
#

PROG=	rump.dhcpclient
SRCS=	main.c bpf.c common.c configure.c dhcp.c if-bsd.c net.c

LDADD+=	-lrumpclient

.include <bsd.prog.mk>

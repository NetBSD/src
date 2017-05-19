#	$NetBSD: Makefile,v 1.2.6.1 2017/05/19 00:22:59 pgoyette Exp $
#

PROG=	rump.dhcpclient
SRCS=	main.c bpf.c common.c configure.c dhcp.c if-bsd.c net.c

LDADD+=	-lrumpclient

.include <bsd.prog.mk>

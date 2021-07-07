#	$NetBSD: Makefile,v 1.4 2021/07/07 11:51:46 martin Exp $
#

PROG=	rump.dhcpclient
SRCS=	main.c bpf.c common.c configure.c dhcp.c if-bsd.c net.c

LDADD+=	-lrumpclient

.if ${RUMP_SANITIZE:Uno} != "no"
LDADD+=	-fsanitize=${RUMP_SANITIZE}
.endif

.include <bsd.prog.mk>

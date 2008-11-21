#	$NetBSD: Makefile,v 1.18 2008/11/21 15:39:29 ad Exp $

.if make(obj)

SUBDIR=	libhack more ssh tls \
	x_dhclient x_ed x_gzip x_ifconfig \
	x_netstat x_ping x_route x_umount zcat

.if ${MACHINE} == "acorn32"
SUBDIR+=sparkcrc
.endif

# IPv6
SUBDIR+=x_ping6

.endif

.if exists(sysinst/arch/${MACHINE})
SUBDIR+=sysinst
.endif

release:

.include <bsd.subdir.mk>

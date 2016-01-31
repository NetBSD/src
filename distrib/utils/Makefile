#	$NetBSD: Makefile,v 1.22 2016/01/31 18:56:49 christos Exp $

.if make(obj)

SUBDIR=	libhack more ssh tls \
	x_disklabel x_ed x_gzip x_ifconfig \
	x_netstat x_ping x_route x_umount zcat

.if ${MACHINE} == "acorn32"
SUBDIR+=sparkcrc
.endif

.if ${MACHINE_ARCH} == "m68000" || ${MACHINE_ARCH} == "m68k"
SUBDIR+=edlabel
.endif

# IPv6
SUBDIR+=x_ping6

.endif

release:

.include <bsd.subdir.mk>

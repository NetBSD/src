#	$NetBSD: Makefile,v 1.19 2010/03/10 23:13:10 abs Exp $

.if make(obj)

SUBDIR=	libhack more ssh tls \
	x_dhclient x_ed x_gzip x_ifconfig \
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

.if exists(sysinst/arch/${MACHINE})
SUBDIR+=sysinst
.endif

release:

.include <bsd.subdir.mk>

#	$NetBSD: Makefile,v 1.1.1.1 1996/10/03 06:33:25 mrg Exp $

# XXX - are these useful for users?
#SUBDIR=	dhclient

PROG=	dhcpd 
SRCS=	options.c errwarn.c convert.c tree.c memory.c alloc.c print.c hash.c \
	tables.c inet.c dispatch.c bpf.c packet.c raw.c nit.c socket.c dhcpd.c \
	dhcp.c bootp.c conflex.c confpars.c db.c
MAN=	dhcpd.8 dhcpd.conf.5

.include <bsd.subdir.mk>
.include <bsd.prog.mk>

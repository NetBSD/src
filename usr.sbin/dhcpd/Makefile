#	$NetBSD: Makefile,v 1.3 1996/11/18 10:15:09 mrg Exp $

# XXX - is this useful for users?
#SUBDIR=	dhclient

PROG=	dhcpd 
SRCS=	options.c errwarn.c convert.c tree.c memory.c alloc.c print.c hash.c \
	tables.c inet.c dispatch.c bpf.c packet.c raw.c nit.c socket.c dhcpd.c \
	dhcp.c bootp.c conflex.c confpars.c db.c
MAN=	dhcpd.8 dhcpd.conf.5

.include <bsd.prog.mk>

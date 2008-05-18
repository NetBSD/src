# $NetBSD: Makefile,v 1.22.24.1 2008/05/18 12:36:14 yamt Exp $

LIBISPRIVATE=	yes

LIB    = dhcp
SRCS   = raw.c parse.c nit.c icmp.c dispatch.c conflex.c upf.c bpf.c socket.c \
	 lpf.c dlpi.c packet.c tr.c ethernet.c memory.c print.c options.c \
	 inet.c tree.c tables.c alloc.c fddi.c ctrace.c \
	 dns.c resolv.c execute.c discover.c comapi.c
MAN    = dhcp-options.5 dhcp-eval.5

DHCPSRCDIR=	common

.include <bsd.lib.mk>

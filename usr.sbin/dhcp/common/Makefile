# $NetBSD: Makefile,v 1.13.2.3 2001/08/16 18:53:28 tv Exp $

MAN    = dhcp-options.5 dhcp-eval.5
LIB    = dhcp
SRCS   = raw.c parse.c nit.c icmp.c dispatch.c conflex.c upf.c bpf.c socket.c \
	 lpf.c dlpi.c packet.c tr.c ethernet.c memory.c print.c options.c \
	 inet.c tree.c tables.c alloc.c fddi.c ctrace.c \
	 dns.c resolv.c execute.c discover.c comapi.c

DHCPSRCDIR=	common

MKLINT=	no
MKPIC=	no
MKPROFILE=no

# only needed during build
libinstall::

.include <bsd.lib.mk>

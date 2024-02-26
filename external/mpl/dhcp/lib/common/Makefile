# $NetBSD: Makefile,v 1.2.8.2 2024/02/26 10:31:27 martin Exp $

LIBISPRIVATE=	yes

LIB    = dhcp
SRCS   = raw.c parse.c nit.c icmp.c dispatch.c conflex.c upf.c bpf.c socket.c \
	 lpf.c dlpi.c packet.c tr.c ethernet.c memory.c print.c options.c \
	 inet.c tree.c tables.c alloc.c fddi.c ctrace.c \
	 dns.c resolv.c execute.c discover.c comapi.c ns_name.c
MAN    = dhcp-options.5 dhcp-eval.5

DHCPSRCDIR=	common

COPTS.discover.c+=	${GCC_NO_STRINGOP_TRUNCATION}
COPTS.parse.c+=		${GCC_NO_FORMAT_TRUNCATION} ${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.lib.mk>

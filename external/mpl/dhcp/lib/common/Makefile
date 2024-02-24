# $NetBSD: Makefile,v 1.2.8.1 2024/02/24 13:07:35 martin Exp $

LIBISPRIVATE=	yes

LIB    = dhcp
SRCS   = raw.c parse.c nit.c icmp.c dispatch.c conflex.c upf.c bpf.c socket.c \
	 lpf.c dlpi.c packet.c tr.c ethernet.c memory.c print.c options.c \
	 inet.c tree.c tables.c alloc.c fddi.c ctrace.c \
	 dns.c resolv.c execute.c discover.c comapi.c ns_name.c
MAN    = dhcp-options.5 dhcp-eval.5

DHCPSRCDIR=	common

COPTS.discover.c+=	${CC_WNO_STRINGOP_TRUNCATION}
COPTS.parse.c+=		${CC_WNO_FORMAT_TRUNCATION} ${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.lib.mk>

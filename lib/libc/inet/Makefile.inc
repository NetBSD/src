#	$NetBSD: Makefile.inc,v 1.3 2026/10/04 03:28:44 mrg Exp $

# net sources
.PATH: ${.PARSEDIR}

SRCS+=	inet_addr.c inet_cidr_ntop.c \
	inet_cidr_pton.c inet_lnaof.c inet_makeaddr.c \
	inet_net_ntop.c inet_net_pton.c inet_neta.c inet_netof.c \
	inet_network.c inet_ntoa.c inet_ntop.c inet_pton.c nsap_addr.c

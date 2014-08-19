#	$NetBSD: Makefile.inc,v 1.1.56.1 2014/08/20 00:02:15 tls Exp $

# net sources
.PATH: ${.CURDIR}/inet

SRCS+=	inet_addr.c inet_cidr_ntop.c \
	inet_cidr_pton.c inet_lnaof.c inet_makeaddr.c \
	inet_net_ntop.c inet_net_pton.c inet_neta.c inet_netof.c \
	inet_network.c inet_ntoa.c inet_ntop.c inet_pton.c nsap_addr.c

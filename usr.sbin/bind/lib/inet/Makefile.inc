# $NetBSD: Makefile.inc,v 1.1.1.1 1998/10/05 18:01:58 tron Exp $

.PATH: ${.CURDIR}/inet

SRCS+=	nsap_addr.c inet_addr.c inet_ntop.c inet_pton.c \
	inet_ntoa.c inet_neta.c inet_net_ntop.c inet_net_pton.c \
	inet_lnaof.c inet_makeaddr.c inet_netof.c inet_network.c

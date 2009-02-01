#	$NetBSD: Makefile.inc,v 1.2 2009/02/01 17:05:26 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../netinet

# IP
SRCS+=	in_proto.c igmp.c in.c in_offload.c in_pcb.c ip_icmp.c		\
	ip_flow.c ip_id.c ip_input.c ip_output.c raw_ip.c in_cksum.c	\
	cpu_in_cksum.c in4_cksum.c ip_encap.c

# ARP
SRCS+=	if_arp.c

# TCP
SRCS+=	tcp_input.c tcp_output.c tcp_sack.c tcp_subr.c tcp_timer.c	\
	tcp_usrreq.c tcp_congctl.c

# UDP
SRCS+=	udp_usrreq.c

CFLAGS+=	-Wno-pointer-sign
CPPFLAGS+=	-I${.CURDIR}/opt -I${.CURDIR}/../libnet/opt
CPPFLAGS+=	-DGATEWAY

# TCP debugging
#SRCS+=		tcp_debug.c
#CPPFLAGS+=	-DTCP_DEBUG

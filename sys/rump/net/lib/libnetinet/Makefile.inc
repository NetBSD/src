#	$NetBSD: Makefile.inc,v 1.8.14.3 2017/12/03 11:39:18 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../netinet

CPPFLAGS+=	-DINET

# INET
SRCS+=	in_proto.c igmp.c in.c in_offload.c in_pcb.c ip_carp.c ip_icmp.c\
	ip_flow.c ip_id.c ip_input.c ip_reass.c ip_output.c raw_ip.c	\
	in_cksum.c cpu_in_cksum.c in4_cksum.c ip_encap.c portalgo.c

# ARP
SRCS+=	if_arp.c

# TCP
SRCS+=	tcp_congctl.c tcp_input.c tcp_output.c tcp_sack.c tcp_subr.c \
	tcp_timer.c tcp_usrreq.c tcp_vtw.c

# UDP
SRCS+=	udp_usrreq.c

CPPFLAGS+=	-DPORTALGO_INET4_DEFAULT=PORTALGO_RANDOM_START

# TCP debugging
#SRCS+=		tcp_debug.c
#CPPFLAGS+=	-DTCP_DEBUG

# IPSEC
SRCS+=			ip_ecn.c
CPPFLAGS+=		-DIPSEC
# To write rump-specific wrappers
CPPFLAGS.in_proto.c+=	-DIPSEC_RUMPKERNEL

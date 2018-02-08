#	$NetBSD: Makefile.inc,v 1.5 2018/02/08 11:34:35 maxv Exp $
#

.PATH:	${.CURDIR}/../../../../netinet6

CPPFLAGS+=	-DINET6

# INET6
SRCS+=	dest6.c frag6.c icmp6.c in6.c in6_cksum.c in6_ifattach.c	\
	in6_offload.c in6_pcb.c in6_proto.c in6_src.c ip6_flow.c	\
	ip6_forward.c ip6_id.c ip6_input.c ip6_mroute.c ip6_output.c	\
	mld6.c nd6.c nd6_nbr.c nd6_rtr.c raw_ip6.c route6.c scope6.c	\
	udp6_usrreq.c

CPPFLAGS+=	-DPORTALGO_INET6_DEFAULT=PORTALGO_RANDOM_START

# IPSEC
CPPFLAGS+=	-DIPSEC
# To write rump-specific wrappers
CPPFLAGS.in6_proto.c+=	-DIPSEC_RUMPKERNEL

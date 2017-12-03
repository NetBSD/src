#	$NetBSD: Makefile,v 1.2.18.2 2017/12/03 11:38:56 jdolecek Exp $
#

.include "../Makefile.inc"

.PATH:	${S}/external/bsd/ipf/netinet

KMOD=	ipl	# Use "formal" name of ipfilter device to enable autoload

# Options for opt_ipfilter.h

CPPFLAGS+=	-DIPFILTER_LOG		# logging of ip packets
#CPPFLAGS+=	-DIPFILTER_DEFAULT_BLOCK
#CPPFLAGS+=	-DIPFILTER_BPF		# BPF opcodes in rules
#CPPFLAGS+=	-DIPFILTER_CKSUM	# check layer 4 checksums
#CPPFLAGS+=	-DIPFILTER_COMPILED	# support for compiled IPF rules
CPPFLAGS+=	-DIPFILTER_LOOKUP	# support for ippool(8)
#CPPFLAGS+=	-DIPFILTER_SCAN		# scanning of packet contents
#CPPFLAGS+=	-DIPFILTER_SYNC		# synchronisation of state 
CPPFLAGS+=	-DIPFILTER_COMPAT	# IPFilter version compat.

# IPFILTER_LOGSIZE	# size of logging buffer
# IPSTATE_MAX		# maximum number of references to state table entry
# IPSTATE_SIZE		# size of state hash table
# NAT_TABLE_MAX		# maximum number of NAT state entries
# NAT_TABLE_SZ		# size of NAT state hash table
# NAT_SIZE		# size of nat rules hash table
# RDR_SIZE		# size of rdr rules hash table
# HOSTMAP_SIZE		# size of hostmap hash table

CPPFLAGS+=	-I$S/external/bsd/ipf

# Other options:

CPPFLAGS+=	-DINET			# opt_inet.h
CPPFLAGS+=	-DINET6			# opt_inet.h
CPPFLAGS+=	-DIPSEC			# opt_ipsec.h

SRCS+=	fil.c
SRCS+=	ip_auth.c
SRCS+=	ip_dstlist.c
SRCS+=	ip_fil_netbsd.c
SRCS+=	ip_frag.c
SRCS+=	ip_htable.c
SRCS+=	ip_log.c
SRCS+=	ip_lookup.c
SRCS+=	ip_nat.c
SRCS+=	ip_nat6.c
SRCS+=	ip_pool.c
SRCS+=	ip_proxy.c
SRCS+=	ip_scan.c
SRCS+=	ip_state.c
SRCS+=	ip_sync.c
SRCS+=	ip_fil_compat.c
SRCS+=	radix_ipf.c

CWARNFLAGS.clang+=	-Wno-self-assign

.include <bsd.kmodule.mk>

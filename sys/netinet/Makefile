#	$NetBSD: Makefile,v 1.1.12.3 1999/11/30 13:35:19 itojun Exp $

KDIR=	/sys/netinet
INCSDIR= /usr/include/netinet

INCS=	icmp_var.h if_atm.h if_ether.h if_inarp.h igmp.h igmp_var.h in.h \
	in_pcb.h in_systm.h in_var.h ip.h ip_auth.h ip_compat.h ip_fil.h \
	ip_frag.h ip_icmp.h ip_mroute.h ip_nat.h ip_proxy.h ip_state.h \
	ip_var.h tcp.h tcp_debug.h tcp_fsm.h tcp_seq.h tcp_timer.h tcp_var.h \
	tcpip.h udp.h udp_var.h
INCS+=	icmp6.h ip6.h in_gif.h ip_ecn.h
INCS+=	altq.h altq_afmap.h altq_blue.h altq_cbq.h altq_cdnr.h altq_classq.h \
	altq_fifoq.h altq_flowvalve.h altq_hfsc.h altq_red.h altq_rio.h \
	altq_rmclass.h altq_rmclass_debug.h altq_var.h altq_wfq.h

.include <bsd.kinc.mk>

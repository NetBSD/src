#	$NetBSD: Makefile,v 1.30.2.2 2017/12/03 11:39:02 jdolecek Exp $

INCSDIR= /usr/include/net

INCS=	bpf.h bpfjit.h bpfdesc.h dlt.h ethertypes.h if.h if_arc.h if_arp.h \
	if_atm.h if_bridgevar.h if_dl.h if_ether.h if_etherip.h if_fddi.h if_gif.h \
	if_gre.h if_hippi.h if_ieee1394.h if_llc.h if_media.h if_mpls.h \
	if_pflog.h if_ppp.h if_pppoe.h if_l2tp.h if_sppp.h if_srt.h if_stf.h \
	if_tap.h if_token.h if_tun.h if_types.h if_vlanvar.h net_stats.h \
	netisr.h pfil.h pfkeyv2.h pfvar.h ppp-comp.h ppp_defs.h radix.h \
	raw_cb.h route.h slcompress.h slip.h zlib.h

SUBDIR=	agr npf

.include <bsd.kinc.mk>

.PATH: ${NETBSDSRCDIR}/sys/dist/pf/net

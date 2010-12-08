#	$NetBSD: Makefile,v 1.1 2010/12/08 20:20:17 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../net/agr

LIB=	rumpnet_agr

SRCS=	if_agr.c if_agrsoftc.c if_agrsubr.c if_agrtimer.c if_agrmonitor.c \
	if_agrether.c if_agrether_hash.c ieee8023ad_marker.c ieee8023ad_lacp.c \
	ieee8023ad_lacp_select.c ieee8023ad_lacp_timer.c \
	ieee8023ad_lacp_sm_rx.c ieee8023ad_lacp_sm_mux.c \
	ieee8023ad_lacp_sm_ptx.c ieee8023ad_lacp_sm_tx.c \
	ieee8023ad_lacp_debug.c ieee8023_tlv.c

SRCS+=	component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

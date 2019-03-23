# $NetBSD: Makefile,v 1.4 2019/03/23 09:48:04 pgoyette Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net/agr

KMOD=		if_agr
IOCONF=		agr.ioconf
SRCS= \
ieee8023_tlv.c \
ieee8023ad_lacp.c \
ieee8023ad_lacp_debug.c \
ieee8023ad_lacp_select.c \
ieee8023ad_lacp_sm_mux.c \
ieee8023ad_lacp_sm_ptx.c \
ieee8023ad_lacp_sm_rx.c \
ieee8023ad_lacp_sm_tx.c \
ieee8023ad_lacp_timer.c \
ieee8023ad_marker.c \
if_agr.c \
if_agrether.c \
if_agrether_hash.c \
if_agrmonitor.c \
if_agrsoftc.c \
if_agrsubr.c \
if_agrtimer.c 

CPPFLAGS+=	-DINET

WARNS=		3

.include <bsd.kmodule.mk>

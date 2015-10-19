#	$NetBSD: Makefile,v 1.4 2015/10/19 16:16:38 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../net80211

LIB=	rumpnet_net80211
COMMENT=IEEE 802.11 (wireless LAN) support

SRCS=   ieee80211.c ieee80211_amrr.c ieee80211_crypto.c			\
	ieee80211_crypto_ccmp.c ieee80211_crypto_none.c			\
	ieee80211_crypto_tkip.c ieee80211_crypto_wep.c ieee80211_input.c\
	ieee80211_ioctl.c ieee80211_netbsd.c ieee80211_node.c		\
	ieee80211_output.c ieee80211_proto.c ieee80211_rssadapt.c	\
	ieee80211_xauth.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

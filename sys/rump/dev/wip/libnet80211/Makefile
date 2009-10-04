#	$NetBSD: Makefile,v 1.1 2009/10/04 10:40:40 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../net80211

LIB=	rumpdev_net80211

SRCS=   ieee80211.c ieee80211_acl.c ieee80211_amrr.c ieee80211_crypto.c	\
	ieee80211_crypto_ccmp.c ieee80211_crypto_none.c			\
	ieee80211_crypto_tkip.c ieee80211_crypto_wep.c ieee80211_input.c\
	ieee80211_ioctl.c ieee80211_netbsd.c ieee80211_node.c		\
	ieee80211_output.c ieee80211_proto.c ieee80211_rssadapt.c	\
	ieee80211_xauth.c

CFLAGS+=	-Wno-pointer-sign
CPPFLAGS+=	-I${.CURDIR}/opt -I${RUMPTOP}/librump/rumpnet/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

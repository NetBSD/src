# $NetBSD: Makefile,v 1.2 2008/01/26 21:50:21 christos Exp $

.include "${.CURDIR}/../Makefile.inc"


PROG=	hostapd
SRCS=	accounting.c ap_list.c beacon.c common.c config.c crypto_libc.c \
	ctrl_iface.c driver_bsd.c drivers.c eap.c eap_common.c \
	eap_identity.c eap_methods.c eapol_sm.c eloop.c hostapd.c \
	hw_features.c ieee802_11.c ieee802_11_auth.c ieee802_11h.c \
	ieee802_1x.c ip_addr.c l2_packet_freebsd.c md5.c mlme.c os_unix.c \
	pmksa_cache.c radius.c radius_client.c rc4.c sha1.c sta_info.c \
	tls_none.c vlan_init.c wme.c wpa.c wpa_auth_ie.c wpa_common.c \
	wpa_debug.c wpabuf.c

.PATH.c: \
	${HOSTAPD_DISTDIR} \
	${WPA_DISTDIR}/src/eap_server \
	${WPA_DISTDIR}/src/radius \
	${.CURDIR}/../wpa_supplicant

MAN=	hostapd.8 hostapd.conf.5

CPPFLAGS+= -I${.CURDIR} -I${HOSTAPD_DISTDIR}
CPPFLAGS+= -DCONFIG_DRIVER_BSD
DPADD+= ${LIBPCAP}
LDADD+= -lpcap

.include <bsd.prog.mk>

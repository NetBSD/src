# $NetBSD: Makefile,v 1.3 2008/10/18 12:32:50 scw Exp $

.include "${.CURDIR}/../Makefile.inc"


PROG=	hostapd
SRCS=	accounting.c aes.c aes_wrap.c ap_list.c beacon.c common.c \
	config.c ctrl_iface.c driver_bsd.c drivers.c eap.c eap_common.c \
	eap_identity.c eap_methods.c eapol_sm.c eloop.c hostapd.c \
	hw_features.c ieee802_11.c ieee802_11_auth.c ieee802_11h.c \
	ieee802_1x.c ip_addr.c l2_packet_freebsd.c md5.c mlme.c os_unix.c \
	pmksa_cache.c radius.c radius_client.c rc4.c sha1.c sta_info.c \
	vlan_init.c wme.c wpa.c wpa_auth_ie.c wpa_common.c wpa_debug.c \
	wpabuf.c

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

.if !defined(NO_CRYPT) && !defined(NO_OPENSSL) && !defined(RELEASE_CRUNCH)
CPPFLAGS+= -DEAP_TLS -DEAP_PEAP -DEAP_MSCHAPv2 -DEAP_PSK \
	   -DEAP_TLS_FUNCS -DEAP_TLS_OPENSSL
SRCS+=	crypto_openssl.c
SRCS+=	eap_tls.c eap_peap.c eap_peap_common.c eap_mschapv2.c \
	eap_psk.c eap_psk_common.c eap_tls_common.c tls_openssl.c \
	ms_funcs.c chap.c

CPPFLAGS+= -DEAP_TTLS -DEAP_MD5
SRCS+=	eap_ttls.c eap_md5.c

CPPFLAGS+= -DEAP_GTC
SRCS+=	eap_gtc.c

CPPFLAGS+= -DEAP_AKA -DEAP_SIM
SRCS+=	eap_aka.c eap_sim.c eap_sim_common.c eap_sim_db.c

CPPFLAGS+= -DEAP_GPSK -DEAP_GPSK_SHA256 -DINTERNAL_SHA256
SRCS+=	eap_gpsk.c eap_gpsk_common.c sha256.c

CPPFLAGS+= -DEAP_PAX
SRCS+=	eap_pax.c eap_pax_common.c

CPPFLAGS+= -DEAP_SAKE
SRCS+=	eap_sake.c eap_sake_common.c

DPADD+= ${LIBSSL} ${LIBCRYPTO} ${LIBDES}
LDADD+= -lssl -lcrypto -ldes
.else
CPPFLAGS+= -DINTERNAL_AES -DINTERNAL_SHA1 -DINTERNAL_MD5
SRCS+=	tls_none.c
.endif

.include <bsd.prog.mk>

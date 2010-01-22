# $NetBSD: Makefile,v 1.12 2010/01/22 16:41:57 sborrill Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	wpa_supplicant
SRCS=	aes.c aes_wrap.c base64.c blacklist.c common.c config.c config_file.c \
	ctrl_iface.c ctrl_iface_unix.c driver_bsd.c drivers.c eap_common.c \
	eap_methods.c eapol_supp_sm.c eloop.c events.c l2_packet_freebsd.c \
	main.c md5.c os_unix.c pmksa_cache.c preauth.c rc4.c scan.c \
	scan_helpers.c sha1.c wpa.c wpa_common.c wpa_debug.c wpa_ie.c \
	wpa_supplicant.c wpabuf.c wpas_glue.c

MAN=	wpa_supplicant.8 wpa_supplicant.conf.5

.PATH.c:  \
	${WPA_SUPPLICANT_DISTDIR} \
	${WPA_DISTDIR}/src/drivers \
	${WPA_DISTDIR}/src/eap_peer \
	${WPA_DISTDIR}/src/rsn_supp

CPPFLAGS+= \
	-I${WPA_SUPPLICANT_DISTDIR} \
	-I${WPA_DISTDIR}/src/drivers \
	-I${WPA_DISTDIR}/src/rsn_supp

CPPFLAGS+= -DCONFIG_DRIVER_BSD
CPPFLAGS+= -DCONFIG_CTRL_IFACE
CPPFLAGS+= -DCONFIG_DEBUG_FILE
DPADD+=	${LIBPCAP}
LDADD+=	-lpcap

.if !defined(NO_ENABLE_WPA_SUPPLICANT_EAPOL)
SRCS+=	eap.c
CPPFLAGS+= -DIEEE8021X_EAPOL

.if !defined(NO_CRYPT) && !defined(NO_OPENSSL) && !defined(RELEASE_CRUNCH)
CPPFLAGS+=-DEAP_TLS -DEAP_PEAP -DEAP_MSCHAPv2 -DEAP_LEAP -DEAP_PSK \
	-DEAP_TLV -DEAP_TLS_FUNCS
SRCS+=	chap.c crypto_openssl.c eap_leap.c eap_mschapv2.c \
	eap_peap.c eap_psk.c eap_psk_common.c eap_tls.c eap_tls_common.c \
	mschapv2.c ms_funcs.c tls_openssl.c eap_peap_common.c

CPPFLAGS+=-DEAP_TTLS -DEAP_MD5
SRCS+=	eap_ttls.c eap_md5.c

# NB: requires patch to openssl
#CPPFLAGS+= -DEAP_FAST
#SRCS+=	eap_fast.c

DPADD+= ${LIBSSL} ${LIBCRYPTO} ${LIBDES}
LDADD+= -lssl -lcrypto -ldes
.else
CPPFLAGS+= -DINTERNAL_AES -DINTERNAL_MD5 -DINTERNAL_SHA1
SRCS+=	tls_none.c
.endif

.endif

.include <bsd.prog.mk>

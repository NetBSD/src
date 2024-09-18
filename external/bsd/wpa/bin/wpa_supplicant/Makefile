# $NetBSD: Makefile,v 1.12 2024/09/18 15:09:30 christos Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	wpa_supplicant
SRCS= \
bss.c \
bssid_ignore.c \
config.c \
config_file.c \
ctrl_iface.c \
ctrl_iface_unix.c \
eap_register.c \
events.c \
main.c \
notify.c \
op_classes.c \
rrm.c \
robust_av.c \
scan.c \
wmm_ac.c \
wpa_supplicant.c \
wpas_glue.c

# common
SRCS+= \
ctrl_iface_common.c \
hw_features_common.c \
ieee802_11_common.c \
ptksa_cache.c \
wpa_common.c \

# crypto
SRCS+= \
random.c \
sha1-prf.c \
sha256.c \
sha256-prf.c \

# drivers
SRCS+= \
driver_bsd.c \
driver_common.c \
driver_wired_common.c \
driver_wired.c \
drivers.c \

# eap_common
SRCS+= \
eap_common.c \

# eap_peer
SRCS+= \
eap_methods.c \

# eapol_supp
SRCS+= \
eapol_supp_sm.c \

# l2_packet
SRCS+= \
l2_packet_freebsd.c \

# rsn_supp
SRCS+= \
pmksa_cache.c \
preauth.c \
wpa.c \
wpa_ie.c \


# util
SRCS+= \
base64.c \
bitfield.c \
common.c \
config_line.c \
eloop.c \
os_unix.c \
wpa_debug.c \
wpabuf.c \

MAN=	wpa_supplicant.8 wpa_supplicant.conf.5

CLEANFILES+= config_line.c

config_line.c: ${WPA_DISTDIR}/src/utils/config.c
	${MKTARGET_CREATE}
	cp ${.ALLSRC} ${.TARGET}

.PATH.c:  \
	${WPA_SUPPLICANT_DISTDIR} \
	${WPA_DISTDIR}/src/drivers \
	${WPA_DISTDIR}/src/eap_peer \
	${WPA_DISTDIR}/src/rsn_supp \
	${COMMON_PATH} \

CPPFLAGS+= \
	-I${WPA_SUPPLICANT_DISTDIR} \
	-I${WPA_DISTDIR}/src/drivers \
	-I${WPA_DISTDIR}/src/rsn_supp

CPPFLAGS+=-DCONFIG_BACKEND_FILE
CPPFLAGS+=-DCONFIG_PEERKEY
CPPFLAGS+=-DCONFIG_MATCH_IFACE
CPPFLAGS+=-DCONFIG_DRIVER_BSD
CPPFLAGS+=-DCONFIG_DRIVER_WIRED
CPPFLAGS+=-DCONFIG_CTRL_IFACE
CPPFLAGS+=-DCONFIG_CTRL_IFACE_UNIX
CPPFLAGS+=-DCONFIG_SMARTCARD
CPPFLAGS+=-DCONFIG_DEBUG_FILE
CPPFLAGS+=-DCONFIG_DEBUG_SYSLOG

#CPPFLAGS+=-DCONFIG_FST
#SRCS+=fst.c \
#fst_ctrl_aux.c \
#fst_ctrl_iface.c \
#fst_group.c \
#fst_iface.c \
#fst_session.c

#CPPFLAGS+=-DCONFIG_ACS
#CPPFLAGS+=-DCONFIG_MBO
#CPPFLAGS+=-DCONFIG_WNM 
#CPPFLAGS+=-DCONFIG_AP
#SRCS+=ap.c \
#gas.c \
#mbo.c 

DPADD+=	${LIBPCAP}
LDADD+=	-lpcap

.if !defined(NO_ENABLE_WPA_SUPPLICANT_EAPOL)
SRCS+=	eap.c
CPPFLAGS+= -DIEEE8021X_EAPOL -DPKCS12_FUNCS

.if !defined(NO_CRYPT) && !defined(NO_OPENSSL) && !defined(RELEASE_CRUNCH)
CPPFLAGS+=-DEAP_TLS
CPPFLAGS+=-DEAP_PEAP
CPPFLAGS+=-DEAP_TTLS
CPPFLAGS+=-DEAP_MD5
CPPFLAGS+=-DEAP_MSCHAPv2
CPPFLAGS+=-DEAP_GTC
CPPFLAGS+=-DEAP_OTP
CPPFLAGS+=-DEAP_LEAP
CPPFLAGS+=-DEAP_PSK
CPPFLAGS+=-DEAP_TLS_OPENSSL
SRCS+= \
chap.c \
crypto_openssl.c \
eap_gtc.c \
eap_leap.c \
eap_md5.c \
eap_mschapv2.c \
eap_otp.c \
eap_peap.c \
eap_peap_common.c \
eap_psk.c \
eap_psk_common.c \
eap_tls.c \
eap_tls_common.c \
eap_ttls.c \
ms_funcs.c \
mschapv2.c \
sha1-tlsprf.c \
tls_openssl.c

SRCS+=aes-ctr.c \
aes-eax.c \
aes-encblock.c \

NOSRCS+= \
aes-omac1.c \

# NB: requires patch to openssl
#CPPFLAGS+= -DEAP_FAST
#SRCS+=	eap_fast.c

DPADD+= ${LIBSSL} ${LIBCRYPTO} ${LIBDES}
LDADD+= -lssl -lcrypto -ldes
.else
CPPFLAGS+= -DINTERNAL_AES -DINTERNAL_MD5 -DINTERNAL_SHA1 -DCONFIG_NO_PBKDF2
SRCS+= tls_none.c sha1-internal.c md5-internal.c aes-xinternal.c rc4.c
CPPFLAGS+= -DCONFIG_CRYPTO_INTERNAL
#SRCS+= aes-unwrap.c md5.c
SRCS+= sha1-pbkdf2.c sha1.c md5.c
.endif

.endif

COPTS.crypto_openssl.c+= -Wno-error=deprecated-declarations
COPTS.tls_openssl.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>

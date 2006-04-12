# $NetBSD: Makefile,v 1.5 2006/04/12 15:35:34 rpaulo Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	wpa_supplicant
SRCS=	aes_wrap.c base64.c common.c config.c config_file.c \
	ctrl_iface.c driver_netbsd.c drivers.c eloop.c events.c \
	l2_packet.c main.c md5.c preauth.c rc4.c sha1.c wpa.c \
	wpa_supplicant.c

MAN=	wpa_supplicant.8 wpa_supplicant.conf.5

.PATH.c: ${.CURDIR}/.. ${WPA_SUPPLICANT_DISTDIR}

CPPFLAGS+= -I${.CURDIR} -I${WPA_SUPPLICANT_DISTDIR}
CPPFLAGS+= -DCONFIG_DRIVER_BSD
CPPFLAGS+= -DCONFIG_CTRL_IFACE
CFLAGS+= -g
DPADD+=	${LIBPCAP}
LDADD+=	-lpcap

.if !defined(NO_ENABLE_WPA_SUPPLICANT_EAPOL)
SRCS+=	eapol_sm.c eap.c
CPPFLAGS+= -DIEEE8021X_EAPOL

.if !defined(NO_CRYPT) && !defined(NO_OPENSSL) && !defined(RELEASE_CRUNCH)
CFLAGS+=-DEAP_TLS -DEAP_PEAP -DEAP_MSCHAPv2 -DEAP_LEAP -DEAP_PSK \
	-DEAP_TLV -DEAP_TLS_FUNCS
SRCS+=	eap_tls.c eap_peap.c eap_mschapv2.c eap_leap.c eap_psk.c \
	eap_psk_common.c eap_tlv.c eap_tls_common.c tls_openssl.c \
	ms_funcs.c crypto.c

CPPFLAGS+=-DEAP_TTLS -DEAP_MD5
SRCS+=	eap_ttls.c eap_md5.c

# NB: requires patch to openssl
#CPPFLAGS+= -DEAP_FAST
#SRCS+=	eap_fast.c

DPADD+= ${LIBSSL} ${LIBCRYPTO} ${LIBDES}
LDADD+= -lssl -lcrypto -ldes
.else
SRCS+=	tls_none.c
.endif

.endif

.include <bsd.prog.mk>

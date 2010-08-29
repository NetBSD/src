# $NetBSD: Makefile,v 1.2 2010/08/29 15:53:01 christos Exp $

.include "${.CURDIR}/../Makefile.inc"


PROG=	hostapd

# ap
SRCS+= \
accounting.c \
ap_config.c \
ap_drv_ops.c \
ap_mlme.c \
authsrv.c \
ctrl_iface_ap.c \
drv_callbacks.c \
hostapd.c \
ieee802_11_auth.c \
ieee802_1x.c \
peerkey_auth.c \
pmksa_cache_auth.c \
preauth_auth.c \
sta_info.c \
tkip_countermeasures.c \
utils.c \
vlan_init.c \
wpa_auth.c \
wpa_auth_glue.c \
wpa_auth_ie.c

# common
SRCS+= \
ieee802_11_common.c \
wpa_common.c \
wpa_ctrl.c

# drivers
SRCS+= \
driver_bsd.c \
drivers.c

# radius
SRCS+= \
radius.c \
radius_client.c \
radius_server.c

# utils
SRCS+= \
base64.c \
common.c \
eloop.c \
ip_addr.c \
os_unix.c \
wpa_debug.c \
wpabuf.c

# main
SRCS+= \
main.c \
config_file.c \
ctrl_iface.c \
dump_state.c \
eap_register.c

# l2_packet
SRCS+= \
l2_packet_freebsd.c

# eapol_auth
SRCS+= \
eapol_auth_dump.c \
eapol_auth_sm.c

# eap_server
SRCS+= \
eap_common.c \
eap_server.c \
eap_server_methods.c

# crypto
SRCS+= \
md5.c \
sha1.c \
sha1-pbkdf2.c

.PATH.c: \
	${HOSTAPD_DISTDIR} \
	${WPA_DISTDIR}/src/eap_server \
	${WPA_DISTDIR}/src/eapol_auth \
	${WPA_DISTDIR}/src/drivers \
	${WPA_DISTDIR}/src/radius

MAN=	hostapd.8 hostapd.conf.5

CPPFLAGS+= -I${.CURDIR} -I${HOSTAPD_DISTDIR}
CPPFLAGS+= -DHOSTAPD
CPPFLAGS+= -DHOSTAPD_DUMP_STATE
CPPFLAGS+= -DALL_DH_GROUPS
CPPFLAGS+= -DRADIUS_SERVER
CPPFLAGS+= -DCONFIG_CTRL_IFACE
CPPFLAGS+= -DCONFIG_CTRL_IFACE_UNIX
CPPFLAGS+= -DCONFIG_DRIVER_BSD
.if ${MKINET6} != "no"
CPPFLAGS+= -DCONFIG_IPV6
.endif
CPPFLAGS+= -DCONFIG_PEERKEY
CPPFLAGS+= -DCONFIG_RSN_PREAUTH

DPADD+= ${LIBPCAP}
LDADD+= -lpcap

.if !defined(NO_CRYPT) && !defined(NO_OPENSSL) && !defined(RELEASE_CRUNCH)
CPPFLAGS+= -DEAP_SERVER
CPPFLAGS+= -DEAP_SERVER_AKA
CPPFLAGS+= -DEAP_SERVER_AKA_PRIME
CPPFLAGS+= -DEAP_SERVER_GPSK
CPPFLAGS+= -DEAP_SERVER_GPSK_SHA256
CPPFLAGS+= -DEAP_SERVER_GTC
CPPFLAGS+= -DEAP_SERVER_IDENTITY
CPPFLAGS+= -DEAP_SERVER_IKEV2
CPPFLAGS+= -DEAP_SERVER_MD5
CPPFLAGS+= -DEAP_SERVER_MSCHAPV2
CPPFLAGS+= -DEAP_SERVER_PAX
CPPFLAGS+= -DEAP_SERVER_PEAP
CPPFLAGS+= -DEAP_SERVER_PSK
CPPFLAGS+= -DEAP_SERVER_SAKE
CPPFLAGS+= -DEAP_SERVER_SIM
CPPFLAGS+= -DEAP_SERVER_TLS
CPPFLAGS+= -DEAP_SERVER_TNC
CPPFLAGS+= -DEAP_SERVER_TTLS
CPPFLAGS+= -DEAP_TLS_FUNCS
CPPFLAGS+= -DPKCS12_FUNCS

# crypto
SRCS+= \
aes-cbc.c \
aes-ctr.c \
aes-eax.c \
aes-encblock.c \
aes-omac1.c \
aes-wrap.c \
crypto_openssl.c \
dh_groups.c \
fips_prf_openssl.c \
ms_funcs.c \
sha1-tlsprf.c \
sha256.c \
tls_openssl.c

# eap_common
SRCS+= \
chap.c \
eap_gpsk_common.c \
eap_ikev2_common.c \
eap_pax_common.c \
eap_peap_common.c \
eap_psk_common.c \
eap_sake_common.c \
eap_sim_common.c \
ikev2_common.c

# eap_server
SRCS+= \
eap_server_aka.c \
eap_server_gpsk.c \
eap_server_gtc.c \
eap_server_identity.c \
eap_server_ikev2.c \
eap_server_md5.c \
eap_server_mschapv2.c \
eap_server_pax.c \
eap_server_peap.c \
eap_server_psk.c \
eap_server_sake.c \
eap_server_sim.c \
eap_server_tls.c \
eap_server_tls_common.c \
eap_server_tnc.c \
eap_server_ttls.c \
eap_sim_db.c \
ikev2.c \
tncs.c


DPADD+= ${LIBSSL} ${LIBCRYPTO} ${LIBDES}
LDADD+= -lssl -lcrypto -ldes
.else
CPPFLAGS+= -DINTERNAL_AES -DINTERNAL_MD5 -DINTERNAL_SHA1 -DCONFIG_NO_PBKDF2
SRCS+=  tls_none.c sha1-internal.c md5-internal.c aes-xinternal.c rc4.c
.endif

.include <bsd.prog.mk>

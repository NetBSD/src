#	$NetBSD: Makefile,v 1.4 2025/01/08 19:59:40 christos Exp $

.include "../Makefile.inc"

PROG=	pppd
SRCS= \
auth.c \
cbcp.c \
ccp.c \
chap-md5.c \
chap.c \
chap_ms.c \
crypto.c \
crypto_ms.c \
demand.c \
eap.c \
eap-tls.c \
ecp.c \
fsm.c \
ipcp.c \
lcp.c \
magic.c \
main.c \
mppe.c \
multilink.c \
options.c \
peap.c \
ppp-des.c \
ppp-md4.c \
ppp-md5.c \
ppp-sha1.c \
session.c \
sys-bsd.c \
tdb.c \
tls.c \
tty.c \
upap.c \
utils.c

PPPDDIR=	${DIST}/pppd

.PATH:	${PPPDDIR}
MAN=	pppd.8
BINMODE=4555
BINOWN=	root

CPPFLAGS+= -I${.CURDIR} -DHAVE_CONFIG_H -DSUPPORT_UTMP -DSUPPORT_UTMPX
CPPFLAGS+= -I${PPPDDIR} -I${DIST}/../include -DSYSCONFDIR=\"/etc\"
CPPFLAGS+= -DPPPD_PLUGIN_DIR=\"/usr/libexec/ppp\"
CPPFLAGS+= -Wno-error=deprecated-declarations
CPPFLAGS+= -Wno-deprecated-declarations

.if (!defined(SMALLPROG) || defined(SMALLPROG_INET6)) && (${USE_INET6} != "no")
CPPFLAGS+= -DINET6
CPPFLAGS+= -DPPP_WITH_IPV6CP
SRCS+=	eui64.c ipv6cp.c
.endif

.if (!defined(SMALLPROG)) && (${USE_PAM} != "no")
CPPFLAGS+= -DUSE_PAM
LDADD+=-lpam ${PAM_STATIC_LDADD}
DPADD+=${LIBPAM} ${PAM_STATIC_DPADD}
.endif

LDADD+= -lpcap -lcrypt -lssl -lcrypto -lutil -Wl,--export-dynamic
DPADD+= ${LIBPCAP} ${LIBCRYPT} ${LIBSSL} ${LIBCRYPTO} ${LIBUTIL}

.for f in chap-md5 chap_ms eap
COPTS.${f}.c+=	-Wno-pointer-sign
.endfor

CWARNFLAGS.clang+=	-Wno-tautological-compare

COPTS.session.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>

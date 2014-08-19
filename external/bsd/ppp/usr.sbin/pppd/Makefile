#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:52:12 tls Exp $

.include "../Makefile.inc"

PROG=	pppd
SRCS=	auth.c cbcp.c ccp.c chap-md5.c chap-new.c chap_ms.c demand.c eap.c \
	ecp.c fsm.c ipcp.c ipxcp.c lcp.c magic.c main.c multilink.c \
	options.c pppcrypt.c session.c sys-bsd.c tdb.c tty.c upap.c utils.c

PPPDDIR=	${DIST}/pppd

.PATH:	${PPPDDIR}
MAN=	pppd.8
BINMODE=4555
BINOWN=	root

CPPFLAGS+= -I${.CURDIR} -DHAVE_PATHS_H -DSUPPORT_UTMP -DSUPPORT_UTMPX -DPLUGIN
CPPFLAGS+= -I${PPPDDIR} -DPPP_FILTER
CPPFLAGS+= -DCBCP_SUPPORT -DUSE_CRYPT -DCHAPMS -DMSLANMAN -DMPPE
CPPFLAGS+= -DHAVE_MULTILINK -DUSE_TDB

.if (!defined(SMALLPROG) || defined(SMALLPROG_INET6)) && (${USE_INET6} != "no")
CPPFLAGS+= -DINET6
SRCS+=	eui64.c ipv6cp.c
.endif

.if (!defined(SMALLPROG)) && (${USE_PAM} != "no")
CPPFLAGS+= -DUSE_PAM
LDADD+=-lpam ${PAM_STATIC_LDADD}
DPADD+=${LIBPAM} ${PAM_STATIC_DPADD}
.endif

LDADD+= -lpcap -lcrypt -lutil -Wl,--export-dynamic
DPADD+= ${LIBPCAP} ${LIBCRYPT} ${LIBUTIL}

.for f in chap-md5 chap_ms eap
COPTS.${f}.c+=	-Wno-pointer-sign
.endfor

CWARNFLAGS.clang+=	-Wno-tautological-compare

.include <bsd.prog.mk>

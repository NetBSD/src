#	$NetBSD: Makefile,v 1.34 2005/02/20 11:12:44 cube Exp $

.include <bsd.own.mk>

PROG=	pppd
SRCS=	auth.c cbcp.c ccp.c chap-md5.c chap-new.c chap_ms.c demand.c eap.c \
	ecp.c fsm.c ipcp.c ipxcp.c lcp.c magic.c main.c multilink.c \
	options.c pppcrypt.c sys-bsd.c tdb.c tty.c upap.c utils.c

PPPDDIR=	${NETBSDSRCDIR}/dist/pppd/pppd

.PATH:	${PPPDDIR}
MAN=	pppd.8
BINMODE=4555
BINOWN=	root

LDADD= -lpcap -lcrypt -lutil -Wl,--export-dynamic
DPADD= ${LIBPCAP} ${LIBCRYPT} ${LIBUTIL}
CPPFLAGS+= -I${.CURDIR} -DHAVE_PATHS_H -DSUPPORT_UTMP -DSUPPORT_UTMPX -DPLUGIN
CPPFLAGS+= -I${PPPDDIR} -DPPP_FILTER
CPPFLAGS+= -DCBCP_SUPPORT -DUSE_CRYPT -DCHAPMS -DMSLANMAN -DMPPE
CPPFLAGS+= -DHAVE_MULTILINK -DUSE_TDB

.if (${USE_INET6} != "no")
CPPFLAGS+= -DINET6
SRCS+=	eui64.c ipv6cp.c
.endif

.include <bsd.prog.mk>

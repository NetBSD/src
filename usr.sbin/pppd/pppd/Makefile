#	$NetBSD: Makefile,v 1.31 2003/01/26 17:34:19 christos Exp $

.include <bsd.own.mk>

PCAPDIR=${NETBSDSRCDIR}/lib/libpcap

PROG=	pppd
SRCS=	auth.c cbcp.c ccp.c chap.c chap_ms.c demand.c eui64.c fsm.c \
	ipcp.c ipv6cp.c ipxcp.c lcp.c magic.c main.c multilink.c options.c \
	sys-bsd.c tdb.c tty.c upap.c utils.c

.PATH:	${PCAPDIR}
MAN=	pppd.8
BINMODE=4555
BINOWN=	root

# don't strip object so that it can load plugins
STRIPFLAG=
LDADD= -lpcap -lcrypt -lutil -Wl,--export-dynamic
DPADD= ${LIBPCAP} ${LIBCRYPT} ${LIBUTIL}
CPPFLAGS+= -I. -DHAVE_PATHS_H -DSUPPORT_UTMP -DSUPPORT_UTMPX -DPLUGIN
CPPFLAGS+= -I${PCAPDIR} -DPPP_FILTER
CPPFLAGS+= -DCBCP_SUPPORT -DCHAPMS -DUSE_CRYPT -DMSLANMAN
CPPFLAGS+= -DINET6

.include <bsd.prog.mk>

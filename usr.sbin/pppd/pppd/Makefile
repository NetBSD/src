#	$NetBSD: Makefile,v 1.26 1999/08/25 02:07:41 christos Exp $

PCAPDIR=${.CURDIR}/../../../lib/libpcap

PROG=	pppd
SRCS=	auth.c cbcp.c ccp.c chap.c chap_ms.c demand.c eui64.c fsm.c \
	ipcp.c ipv6cp.c ipxcp.c lcp.c magic.c main.c options.c sys-bsd.c \
	upap.c utils.c

.PATH:	${PCAPDIR}
MAN=	pppd.8
BINMODE=4555
BINOWN=	root

LDADD=	-lpcap -lcrypt -lutil
DPADD=	${LIBPCAP} ${LIBCRYPT} ${LIBUTIL}
CPPFLAGS+= -I. -DHAVE_PATHS_H 
CPPFLAGS+= -I${PCAPDIR} -DPPP_FILTER
CPPFLAGS+= -DCBCP_SUPPORT -DCHAPMS -DUSE_CRYPT -DMSLANMAN
# XXX: Does not work (yet)
#CPPFLAGS+= -DINET6

.include <bsd.prog.mk>

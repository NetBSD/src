#	$NetBSD: Makefile,v 1.1.1.4 2000/09/23 22:14:58 christos Exp $

PCAPDIR=${.CURDIR}/../../../lib/libpcap

PROG=	pppd
SRCS=	auth.c cbcp.c ccp.c chap.c chap_ms.c demand.c eui64.c fsm.c \
	ipcp.c ipv6cp.c ipxcp.c lcp.c magic.c main.c multilink.c options.c \
	sys-bsd.c tdb.c tty.c upap.c utils.c

.PATH:	${PCAPDIR}
MAN=	pppd.8
BINMODE=4555
BINOWN=	root

LDADD=	-lpcap -lcrypt -lutil
DPADD=	${LIBPCAP} ${LIBCRYPT} ${LIBUTIL}
CPPFLAGS+= -I. -DHAVE_PATHS_H 
CPPFLAGS+= -I${PCAPDIR} -DPPP_FILTER
CPPFLAGS+= -DCBCP_SUPPORT -DCHAPMS -DUSE_CRYPT -DMSLANMAN
CPPFLAGS+= -DINET6

.include <bsd.prog.mk>

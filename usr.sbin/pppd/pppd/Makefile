#	$NetBSD: Makefile,v 1.24 1998/05/02 14:19:13 christos Exp $

PCAPDIR=${.CURDIR}/../../../lib/libpcap

PROG=	pppd
SRCS=	auth.c cbcp.c ccp.c chap.c chap_ms.c demand.c fsm.c ipcp.c \
	ipxcp.c lcp.c magic.c main.c options.c sys-bsd.c upap.c

.PATH:	${PCAPDIR}
MAN=	pppd.8
BINMODE=4555
BINOWN=	root

LDADD=	-lpcap -lcrypt -lutil
DPADD=	${LIBPCAP} ${LIBCRYPT} ${LIBUTIL}
CPPFLAGS+= -I. -DHAVE_PATHS_H -I${PCAPDIR} -DPPP_FILTER -DCBCP_SUPPORT
CPPFLAGS+= -DCHAPMS -DUSE_CRYPT -DMSLANMAN

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.17 1997/05/09 09:36:28 mycroft Exp $

PCAPDIR=${.CURDIR}/../../../lib/libpcap

PROG=	pppd
SRCS=	auth.c cbcp.c ccp.c chap.c chap_ms.c demand.c fsm.c ipcp.c \
	ipxcp.c lcp.c magic.c main.c options.c sys-bsd.c upap.c

.PATH:	${PCAPDIR}
MAN=	pppd.8 pppd.cbcp.8
BINMODE=4555
BINOWN=	root

LDADD=	-lpcap -lcrypt -lutil
DPADD=	${LIBPCAP} ${LIBCRYPT} ${LIBUTIL}
CFLAGS+= -I. -DHAVE_PATHS_H -I${PCAPDIR} -DPPP_FILTER
#CFLAGS+= -Wall -Wstrict-prototypes -Wmissing-prototypes -Dlint

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.1.1.3 1997/09/26 18:51:07 christos Exp $

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
CFLAGS+= -Wall -Wstrict-prototypes -Wmissing-prototypes

.include "../../Makefile.inc"
.include <bsd.prog.mk>

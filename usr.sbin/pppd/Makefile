#	$NetBSD: Makefile,v 1.13 1996/12/18 16:54:57 christos Exp $

PCAPDIR=${.CURDIR}/../../lib/libpcap

PROG=	pppd
SRCS=	main.c magic.c fsm.c lcp.c ipcp.c upap.c chap.c md5.c ccp.c \
	auth.c options.c sys-bsd.c demand.c grammar.c scanner.c

.PATH:	${PCAPDIR} ${.CURDIR}/../../sys/net
MAN=	pppd.8
SUBDIR=	pppstats chat
BINMODE=4555
BINOWN=	root

LDADD=	-lpcap -lcrypt -lutil
DPADD=	${LIBPCAP} ${LIBCRYPT} ${LIBUTIL}
CFLAGS+= -I. -I${.CURDIR} -DHAVE_PATHS_H -I${PCAPDIR}
CLEANFILES=y.tab.h grammar.c scanner.c

.include <bsd.prog.mk>

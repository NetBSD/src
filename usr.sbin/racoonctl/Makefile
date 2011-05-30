# $NetBSD: Makefile,v 1.6 2011/05/30 14:41:27 joerg Exp $

WARNS?=	0	# XXX third-party program, many issues
NOCLANGERROR=	# defined

.include <bsd.own.mk>

PROG=	racoonctl
SRCS=	racoonctl.c str2val.c kmpstat.c vmbuf.c sockmisc.c misc.c
MAN=	racoonctl.8

DIST=	${NETBSDSRCDIR}/crypto/dist/ipsec-tools
#CFLAGS+=-g
CPPFLAGS+=	-I${DIST}/src/racoon -I${DIST}/src/libipsec	\
		-I${NETBSDSRCDIR}/lib/libipsec 			\
		-UIPSEC_DEBUG -DHAVE_CONFIG_H -DNOUSE_PRIVSEP	\
		-DADMINPORTDIR=\"/var/run\"
LDADD+=	-lipsec
DPADD+=	${LIBIPSEC}
CLEANFILES+=	racoonctl.cat8

.PATH:  ${NETBSDSRCDIR}/lib/libipsec ${DIST}/src/racoon

.include <bsd.prog.mk>


# $NetBSD: Makefile,v 1.2.10.1 2009/05/13 19:20:36 jym Exp $

WARNS?=	0	# XXX third-party program, many issues

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

